/*=========================================================================

   Program: ParaView
   Module:    pqTemporalExportReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqTemporalExportReaction.h"

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVConfig.h"
#include "vtkRenderWindow.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMExportProxyDepot.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVectorOperators.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"

#include <QWidget>

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

// clang-format off
namespace
{
static const char* tp_python_export_code = R"DONTPARSE(
# boolean telling if we want to export rendering.
export_rendering = %1

# string->string map with key being the proxyname while value being the
# file name on the system the generated python script is to be run on.
reader_input_map = { %2 };

# list of views along with a file name and magnification flag
screenshot_info = {%3}

# the number of processes working together on a single time step
timeCompartmentSize = %4

# makes a cinema D index table
make_cinema_table=%5

# the name of the Python script to be output
scriptFileName = "%6"

# -----------------------------------------------------------------------------
from paraview import smtrace, servermanager, smstate, cpstate

# -----------------------------------------------------------------------------
class ReaderFilter(smtrace.PipelineProxyFilter):
    def should_never_trace(self, prop):
        if smtrace.PipelineProxyFilter.should_never_trace(self, prop): return True
        # skip filename related properties.
        return prop.get_property_name() in [\
            "FilePrefix", "XMLFileName", "FilePattern", "FileRange", "FileName",
            "FileNames"]

# -----------------------------------------------------------------------------
class ReaderAccessor(smtrace.RealProxyAccessor):
    """This accessor is created instead of the standard one for proxies that
    are the temporal readers. This accessor override the
    trace_ctor() method to trace the constructor as the RegisterReader() call,
    since the proxy is a dummy, in this case.
    """
    def __init__(self, varname, proxy, filenameglob):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.FileNameGlob = filenameglob

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        # FIXME: ensures that FileName doesn't get traced.

        # change to call STP.CreateReader instead.
        ctor_args = "%s, fileInfo='%s'" % (ctor, self.FileNameGlob)
        ctor = "STP.CreateReader"
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, ReaderFilter(), ctor_args, skip_assignment)

        trace = smtrace.TraceOutput()
        trace.append(original_trace)
        trace.append(\
            "timeSteps = %s.TimestepValues if len(%s.TimestepValues) != 0 else [0]" % (self, self))
        return trace.raw_data()

# -----------------------------------------------------------------------------
class ViewAccessor(smtrace.RealProxyAccessor):
    """Accessor for views. Overrides trace_ctor() to trace registering of the
    view with the STP.
    """
    def __init__(self, varname, proxy, screenshot_info):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.ScreenshotInfo = screenshot_info

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        trace.append_separated(["# register the view with coprocessor",
          "# and provide it with information such as the filename to use,",
          "# how frequently to write the images, etc."])
        params = self.ScreenshotInfo
        assert len(params) == 4
        trace.append([
            "STP.RegisterView(%s," % self,
            "    filename='%s', magnification=%s, width=%s, height=%s, tp_views=tp_views)" %\
                (params[0], params[1], params[2], params[3])])
        trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class WriterAccessor(smtrace.RealProxyAccessor):
    """Accessor for writers. Overrides trace_ctor() to use the actual writer
    proxy name instead of the dummy-writer proxy's name.
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)

    def get_proxy_label(self, xmlgroup, xmlname):
        pxm = servermanager.ProxyManager()
        prototype = pxm.GetPrototypeProxy(xmlgroup, xmlname)
        if not prototype:
            # a bit of a hack but we assume that there's a stub of some
            # writer that's not available in this build but is available
            # with the build used by the simulation code (probably through a plugin)
            # this stub must have the proper name in the coprocessing hints
            print("WARNING: Could not find %s writer in %s" \
                "XML group. This is not a problem as long as the writer is available with " \
                "the ParaView build used by the simulation code." % (xmlname, xmlgroup))
            ctor = servermanager._make_name_valid(xmlname)
        else:
            ctor = servermanager._make_name_valid(prototype.GetXMLLabel())
        # TODO: use servermanager.ProxyManager().NewProxy() instead
        # we create the writer proxy such that it is not registered with the
        # ParaViewPipelineController, so its state is not sent to ParaView Live.
        return "servermanager.%s.%s" % (xmlgroup, ctor)

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        xmlElement = self.get_object().GetHints().FindNestedElementByName("WriterProxy")
        xmlgroup = xmlElement.GetAttribute("group")
        xmlname = xmlElement.GetAttribute("name")

        filename = self.get_object().FileName
        ctor = self.get_proxy_label(xmlgroup, xmlname)
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)

        trace = smtrace.TraceOutput(original_trace)
        trace.append_separated(["# register the writer with STP",
          "# and provide it with information such as the filename to use"])
        trace.append("STP.RegisterWriter(%s, '%s', tp_writers)" % \
            (self, filename))
        trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class tpstate_filter_proxies_to_serialize(object):
    """filter used to skip views and representations when export_rendering is
    disabled."""
    def __call__(self, proxy):
        global export_rendering
        if not smstate.visible_representations()(proxy):
            return False
        if (not export_rendering) and \
            (proxy.GetXMLGroup() in ["views", "representations"]):
            return False
        return True

# -----------------------------------------------------------------------------
def tp_hook(varname, proxy):
    global export_rendering, screenshot_info, reader_input_map
    """callback to create our special accessors instead of the default ones."""
    pname = smtrace.Trace.get_registered_name(proxy, "sources")
    if pname:
        if pname in reader_input_map:
            # this is a reader.
            return ReaderAccessor(varname, proxy, reader_input_map[pname])
        elif proxy.GetHints() and proxy.GetHints().FindNestedElementByName("WriterProxy"):
            return WriterAccessor(varname, proxy)
        raise NotImplementedError
    pname = smtrace.Trace.get_registered_name(proxy, "views")
    if pname:
        # since view is being accessed, ensure that we were indeed saving
        # rendering components.
        assert export_rendering
        return ViewAccessor(varname, proxy, screenshot_info[pname])
    raise NotImplementedError

# Start trace
filter = tpstate_filter_proxies_to_serialize()
smtrace.RealProxyAccessor.register_create_callback(tp_hook)
state = smstate.get_state(filter=filter, raw=True)
smtrace.RealProxyAccessor.unregister_create_callback(tp_hook)

# add in the new style writer proxies
state = state + cpstate.NewStyleWriters(make_temporal_script=True).make_trace()
pipelineClassDef = "\n"
for original_line in state:
    for line in original_line.split("\n"):
       pipelineClassDef += line + "\n"

output_contents = """
from paraview.simple import *
from paraview import spatiotemporalparallelism as STP

tp_writers = []
tp_views = []

timeCompartmentSize = %s
globalController, temporalController, timeCompartmentSize = STP.CreateControllers(timeCompartmentSize)

makeCinemaTable = %s

%s

# Lastly, run though the local timecompartment on each set of nodes
STP.IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, tp_writers, tp_views, make_cinema_table=makeCinemaTable)
""" % (timeCompartmentSize, make_cinema_table, pipelineClassDef)


outFile = open(scriptFileName, 'w')
outFile.write(output_contents)
outFile.close()
)DONTPARSE";
}
// clang-format on

//-----------------------------------------------------------------------------
pqTemporalExportReaction::pqTemporalExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqTemporalExportReaction::~pqTemporalExportReaction()
{
}

//-----------------------------------------------------------------------------
void pqTemporalExportReaction::onTriggered()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMExportProxyDepot* ed = pxm->GetExportDepot();

  // We populate these from information from the export proxies
  QString export_rendering = "False";
  QString
    sim_inputs_map; // a map from the simulation inputs in the gui to the adaptor's named inputs
  QString rendering_info; // a map from the render view name to render output params
  QString make_cinema_table = "False";
  QString timeInputFilePattern = "";
  int timeCompartmentSize = 1;

  // use export proxy state to populate the rest
  // global options
  vtkSMProxy* globaloptions = ed->GetGlobalOptions();
  timeInputFilePattern = vtkSMPropertyHelper(globaloptions, "TimeInputFilePattern").GetAsString(0);
  timeCompartmentSize = vtkSMPropertyHelper(globaloptions, "TimeCompartmentSize").GetAsInt(0);
  make_cinema_table =
    vtkSMPropertyHelper(globaloptions, "SaveDTable").GetAsInt(0) == 0 ? "False" : "True";

  // populate the reader list
  QList<pqPipelineSource*> sources =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* source, sources)
  {
    if (qobject_cast<pqPipelineFilter*>(source))
    {
      continue;
    }
    QString sourceFormat = "'%1':'%2'";
    sim_inputs_map += sourceFormat.arg(source->getSMName(), timeInputFilePattern) + ",";
  }

  // writers
  bool exported_any_writers = false;
  ed->InitNextWriterProxy();
  while (auto nextWriter = ed->GetNextWriterProxy())
  {
    if (!nextWriter->HasAnnotation("enabled") ||
      strcmp(nextWriter->GetAnnotation("enabled"), "1") != 0)
    {
      continue;
    }

    if (strcmp(nextWriter->GetXMLName(), "Cinema image options") != 0)
    {
      exported_any_writers = true;
    }
  }

  // screenshots
  bool exported_any_screenshots = false;
  ed->InitNextScreenshotProxy();
  while (auto nextScreenshot = ed->GetNextScreenshotProxy())
  {
    vtkSMSaveScreenshotProxy* ssProxy = vtkSMSaveScreenshotProxy::SafeDownCast(nextScreenshot);
    if (!ssProxy->HasAnnotation("enabled") || strcmp(ssProxy->GetAnnotation("enabled"), "1") != 0)
    {
      continue;
    }
    exported_any_screenshots = true;

    std::string filename = vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").GetAsString();
    QString imagefilename = QString::fromStdString(filename);

    vtkSMProxy* viewProxy = ssProxy->GetView();
    QString viewname = pxm->GetProxyName("views", viewProxy);

    // this was catalyst specific, I am dropping it from the UI for now
    QString magnification = "1";

    vtkVector2i targetSize;
    vtkSMPropertyHelper(ssProxy, "ImageResolution").Get(targetSize.GetData(), 2);
    QString width = QString::fromStdString(std::to_string(targetSize.GetX()));
    QString height = QString::fromStdString(std::to_string(targetSize.GetY()));

    QString viewformat = "'%1' : ['%2', %3, %4, %5]";
    QString nextview =
      viewformat.arg(viewname).arg(imagefilename).arg(magnification).arg(width).arg(height);
    rendering_info += nextview + ",";
  }

  if (!(exported_any_screenshots || exported_any_writers))
  {
    qWarning("Nothing to export, use Export Inspector to configure what you want to write.");
  }
  else
  {
    QString filters = "ParaView Python State Files (*.py);;All files (*)";

    pqFileDialog file_dialog(
      NULL, parentAction()->parentWidget(), tr("Save Server State:"), QString(), filters);
    file_dialog.setObjectName("ExportTemporalStateFileDialog");
    file_dialog.setFileMode(pqFileDialog::AnyFile);
    if (!file_dialog.exec())
    {
      return;
    }

    QString filename = file_dialog.getSelectedFiles()[0];
#ifdef _WIN32
    // Convert to forward slashes. The issue is that the path is interpreted as a
    // Python string when passed to the interpreter, so a path such as "C:\tests"
    // is read as "C:<TAB>ests" which isn't what we want. Since Windows is
    // flexible anyways, just use Unix separators.
    filename.replace('\\', '/');
#endif

    QString command = tp_python_export_code;
    export_rendering = (exported_any_screenshots ? "True" : "False");

    sim_inputs_map.chop(1); // remove last ","
    rendering_info.chop(1);

    command = command.arg(export_rendering)
                .arg(sim_inputs_map)
                .arg(rendering_info)
                .arg(timeCompartmentSize)
                .arg(make_cinema_table)
                .arg(filename);

    // ensure Python in initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(command.toLocal8Bit().data());
  }
#endif
}
