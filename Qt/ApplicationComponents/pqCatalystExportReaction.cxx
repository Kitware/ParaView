/*=========================================================================

   Program: ParaView
   Module:    pqCatalystExportReaction.cxx

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
#include "pqCatalystExportReaction.h"

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

#include <iostream>
#include <sstream>
#include <string>

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

namespace
{
static const char* cp_python_export_code = "from paraview import cpexport\n"
                                           "cpexport.DumpCoProcessingScript(export_rendering=%1,\n"
                                           "   simulation_input_map={%2},\n"
                                           "   screenshot_info={%3},\n"
                                           "   padding_amount=%4,\n"
                                           "   rescale_data_range=%5,\n"
                                           "   enable_live_viz=%6,\n"
                                           "   live_viz_frequency=%7,\n"
                                           "   cinema_tracks={%8},\n"
                                           "   filename='%9',\n"
                                           "   cinema_arrays={%10},\n"
                                           "   write_start=%11,\n"
                                           "   make_cinema_table=%12,\n"
                                           "   root_directory='%13',\n"
                                           "   request_specific_arrays=%14,\n"
                                           "   force_first_output=%15)\n";
}

//-----------------------------------------------------------------------------
pqCatalystExportReaction::pqCatalystExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqCatalystExportReaction::~pqCatalystExportReaction()
{
}

//-----------------------------------------------------------------------------
void pqCatalystExportReaction::onTriggered()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter

  // We populate these from information from the export proxies
  QString live_visualization = "True";
  int live_visualization_frequency = 1;

  int write_start = 0;
  QString paddingAmount = "0";

  QString export_rendering = "False";
  QString
    sim_inputs_map; // a map from the simulation inputs in the gui to the adaptor's named inputs
  QString rendering_info; // a map from the render view name to render output params
  QString rescale_data_range = "False";
  QString make_cinema_table = "False";
  QString root_directory = "";
  QString request_specific_arrays = "False";
  QString force_first_output = "False";

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMExportProxyDepot* ed = pxm->GetExportDepot();

  // populate the simulation inputs map
  QList<pqPipelineSource*> sources =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* source, sources)
  {
    if (qobject_cast<pqPipelineFilter*>(source) ||
      vtkSMCoreUtilities::GetFileNameProperty(source->getProxy()) == nullptr)
    {
      continue;
    }
    QString mapentry = source->getSMName() + ":" + source->getSMName();
    QString sourceFormat = "'%1':'%1'";
    sim_inputs_map += sourceFormat.arg(source->getSMName()) + ",";
  }

  // use export proxy state to populate the rest
  // global options
  vtkSMProxy* globaloptions = ed->GetGlobalOptions();
  live_visualization =
    vtkSMPropertyHelper(globaloptions, "EnableLive").GetAsInt(0) == 0 ? "False" : "True";
  live_visualization_frequency = vtkSMPropertyHelper(globaloptions, "LiveFrequency").GetAsInt(0);
  write_start = vtkSMPropertyHelper(globaloptions, "WriteStart").GetAsInt(0);
  paddingAmount = QString::fromStdString(
    std::to_string(vtkSMPropertyHelper(globaloptions, "FileNamePadding").GetAsInt(0)));
  rescale_data_range =
    vtkSMPropertyHelper(globaloptions, "RescaleToDataRange").GetAsInt(0) == 0 ? "False" : "True";
  make_cinema_table =
    vtkSMPropertyHelper(globaloptions, "SaveDTable").GetAsInt(0) == 0 ? "False" : "True";
  root_directory = vtkSMPropertyHelper(globaloptions, "RootDirectory").GetAsString(0);
  request_specific_arrays =
    vtkSMPropertyHelper(globaloptions, "RequestSpecificArrays").GetAsInt(0) == 0 ? "False" : "True";
  force_first_output =
    vtkSMPropertyHelper(globaloptions, "ForceFirstOutput").GetAsInt(0) == 0 ? "False" : "True";

  // writers
  bool exported_any_writers = false;
  QString cinema_tracks = "";
  bool hasTrackSets = false;
  QString cinema_arrays = "";
  bool hasArraySets = false;
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
      continue;
    }

    // done here except cinema image specific parameters
    const char* hashname = pxm->GetProxyName("export_writers", nextWriter);
    std::string formatS = hashname;
    size_t underP = formatS.find_first_of("|");
    std::string filterName = formatS.substr(0, underP);
    filterName[0] = tolower(filterName[0]);

    std::string track = vtkSMPropertyHelper(nextWriter, "Cinema Parameters").GetAsString(0);
    if (track.size())
    {
      hasTrackSets = true;
      QString thisTrack = QString("'");
      thisTrack += QString(filterName.c_str());
      thisTrack += QString("':[");
      thisTrack += QString(track.c_str());
      thisTrack += QString("],");
      cinema_tracks += thisTrack;
    }

    QString theseArrays = QString("'");
    theseArrays += QString(filterName.c_str());
    theseArrays += QString("':[");

    // TODO: there isn't an API to distinguish cell and point arrays or the same name
    bool hasArrays = false;
    int nelems;
    nelems = vtkSMPropertyHelper(nextWriter, "Cell Arrays").GetNumberOfElements();
    for (int i = 0; i < nelems; ++i)
    {
      hasArrays = true;
      theseArrays += "'";
      theseArrays += vtkSMPropertyHelper(nextWriter, "Cell Arrays").GetAsString(i);
      theseArrays += "',";
    }

    nelems = vtkSMPropertyHelper(nextWriter, "Point Arrays").GetNumberOfElements();
    for (int i = 0; i < nelems; ++i)
    {
      hasArrays = true;
      theseArrays += "'";
      theseArrays += vtkSMPropertyHelper(nextWriter, "Point Arrays").GetAsString(i);
      theseArrays += "',";
    }
    if (hasArrays)
    {
      hasArraySets = true;
      theseArrays.chop(1);
      theseArrays += "],";
      cinema_arrays += theseArrays;
    }
  }
  if (hasTrackSets)
  {
    cinema_tracks.chop(1);
  }

  if (hasArraySets)
  {
    cinema_arrays.chop(1);
  }

  // screenshots
  /*
   * Make a string containing a comma separated set of views with each
   * view defined as in 'format'.
   * Order of view values:
   * 1. View name
   * 2. Image file name
   * 3. Frequency
   * 4. Fit to screen
   * 5. Magnification
   * 6. Image width
   * 7. Image height
   * 8. Cinema specific options (dictionary; phi, theta, composite, etc..)
   *
   * Example: Format as defined in pqCinemaConfiguration
   * format = "'%1' : ['%2', %3, %4, %5, %6, %7, %8]"
   * returns -> 'ViewName1' : ['Imname', 1, 1, 1, 1, 1, {'composite': True ...}],
   *            'ViewName2' : [...],
   *            ... (for N views)
   */
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

    int wf = vtkSMPropertyHelper(ssProxy, "WriteFrequency").GetAsInt();
    QString frequency = QString::number(wf);

    // this is catalyst specific, need to add to ssProxy
    QString fitScreen =
      vtkSMPropertyHelper(ssProxy, "CatalystFitToScreen").GetAsInt(0) == 0 ? "0" : "1";

    // this was catalyst specific, I am dropping it from the UI for now
    QString magnification = "1";

    vtkVector2i targetSize;
    vtkSMPropertyHelper(ssProxy, "ImageResolution").Get(targetSize.GetData(), 2);
    QString width = QString::fromStdString(std::to_string(targetSize.GetX()));
    QString height = QString::fromStdString(std::to_string(targetSize.GetY()));

    int compression = -1;
    vtkSMProxy* writerProxy = ssProxy->GetFormatProxy(filename);
    if (writerProxy)
    {
      compression = vtkSMPropertyHelper(writerProxy, "CompressionLevel", true).GetAsInt(0);
    }

    // this is catalyst specific, need to add to SSSProxy
    // but will come from a new CinemaSpecific file format
    QString cinema_options = "{}";

    if (imagefilename.endsWith("cdb"))
    {
      // get the cinema database shape from the CDB subproxy
      if (writerProxy)
      {
        imagefilename = imagefilename.replace("cdb", "png"); // just always use png for normal
                                                             // images
        int renderingLevel = vtkSMPropertyHelper(writerProxy, "DeferredRendering").GetAsInt();
        QString compositeState;
        QString useValues;
        QString valueFormat = "'floatValues':True"; // now that we can rely on float textures
                                                    // everywhere, never use the 0..2^24 hack
        switch (renderingLevel)
        {
          case 0:
          default:
            // spec A
            compositeState = "'composite':False";
            useValues = "'noValues':True";
            break;
          case 1:
            // spec C with RGB colors
            compositeState = "'composite':True";
            useValues = "'noValues':True";
            break;
          case 2:
            // spec C with float colors
            compositeState = "'composite':True";
            useValues = "'noValues':False";
            break;
        }
        int cameraLevel = vtkSMPropertyHelper(writerProxy, "CameraModel").GetAsInt();
        if (renderingLevel == 0 && cameraLevel > 1)
        {
          cameraLevel = 1;
          qWarning(
            "Camera Type choice is not compatible with Cinema Spec A, falling back to phi-theta.");
        }

        int numPhis = vtkSMPropertyHelper(writerProxy, "Camera Phi Divisions").GetAsInt();
        int numThetas = vtkSMPropertyHelper(writerProxy, "Camera Theta Divisions").GetAsInt();
        int numRolls = vtkSMPropertyHelper(writerProxy, "Camera Roll Divisions").GetAsInt();
        if (numRolls > 1)
        {
          if (renderingLevel == 0)
          {
            numRolls = 1;
            qWarning("Roll is not compatible with Cinema Spec A, falling back to roll = 1.");
          }
          else if (cameraLevel < 2)
          {
            numRolls = 1;
            qWarning("Roll requires pose based camera, falling back to roll = 1.");
          }
        }
        QString camState;
        QString phis = "";
        QString thetas = "";
        QString rolls = "";
        switch (cameraLevel)
        {
          case 0:
          default:
            camState = "'camera':'static'";
            break;
          case 1:
            camState = "'camera':'phi-theta'";
            if (numPhis < 2)
            {
              phis = "'phi':[0]";
            }
            else
            {
              phis = "'phi':[";
              for (int j = -180; j < 180; j += (360 / numPhis))
              {
                phis += QString::number(j) + ",";
              }
              phis.chop(1);
              phis += "]";
            }

            if (numThetas < 2)
            {
              thetas = "'theta':[0]";
            }
            else
            {
              thetas = "'theta':[";
              for (int j = -90; j < 90; j += (180 / numThetas))
              {
                thetas += QString::number(j) + ",";
              }
              thetas.chop(1);
              thetas += "]";
            }
            break;
          case 2:
            camState = "'camera':'azimuth-elevation-roll'";
            phis = "'phi':[" + QString::number(numPhis) + "]";
            thetas = "'theta':[" + QString::number(numThetas) + "]";
            rolls = "'roll':[" + QString::number(numRolls) + "]";
            break;
          case 3:
            camState = "'camera':'yaw-pitch-roll'";
            phis = "'phi':[" + QString::number(numPhis) + "]";
            thetas = "'theta':[" + QString::number(numThetas) + "]";
            rolls = "'roll':[" + QString::number(numRolls) + "]";
            break;
        }
        cinema_options =
          "{" + compositeState + "," + useValues + "," + valueFormat + "," + camState;
        if (phis != "")
        {
          cinema_options += "," + phis;
        }
        if (thetas != "")
        {
          cinema_options += "," + thetas;
        }
        if (rolls != "")
        {
          cinema_options += "," + rolls;
        }

        QString trackedObject = vtkSMPropertyHelper(writerProxy, "TrackObject").GetAsString();
        if (trackedObject != "None")
        {
          if (renderingLevel == 0)
          {
            trackedObject = "None";
            qWarning(
              "Object tracking is not compatible with Cinema Spec A, falling back to no tracking.");
          }
          else if (cameraLevel < 2)
          {
            trackedObject = "None";
            qWarning("Object tracking requires pose based camera, falling back to no tracking.");
          }
        }

        cinema_options += ", 'tracking':{ 'object':'" + trackedObject + "'}";

        vtkSMRenderViewProxy* rvp = vtkSMRenderViewProxy::SafeDownCast(viewProxy);
        vtkCamera* cam = rvp->GetActiveCamera();
        double eye[3];
        double at[3];
        double up[3];
        cam->GetPosition(eye);
        vtkSMPropertyHelper(rvp, "CenterOfRotation").Get(at, 3);
        cam->GetViewUp(up);
        cinema_options += ", 'initial':{ ";
        cinema_options += "'eye': [" + QString::number(eye[0]) + "," + QString::number(eye[1]) +
          "," + QString::number(eye[2]) + "], ";
        cinema_options += "'at': [" + QString::number(at[0]) + "," + QString::number(at[1]) + "," +
          QString::number(at[2]) + "], ";
        cinema_options += "'up': [" + QString::number(up[0]) + "," + QString::number(up[1]) + "," +
          QString::number(up[2]) + "] ";
        cinema_options += "}";

        cinema_options += "}";
      }
    }

    QString viewformat = "'%1' : ['%2', %3, %4, %5, %6, %7, %8, %9]";
    QString nextview = viewformat.arg(viewname)
                         .arg(imagefilename)
                         .arg(frequency)
                         .arg(fitScreen)
                         .arg(magnification)
                         .arg(width)
                         .arg(height)
                         .arg(cinema_options)
                         .arg(compression);
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
    file_dialog.setObjectName("ExportCoprocessingStateFileDialog");
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

    QString command = cp_python_export_code;
    export_rendering = (exported_any_screenshots ? "True" : "False");

    sim_inputs_map.chop(1); // remove last ","
    rendering_info.chop(1);
    command = command.arg(export_rendering)
                .arg(sim_inputs_map)
                .arg(rendering_info)
                .arg(paddingAmount)
                .arg(rescale_data_range)
                .arg(live_visualization)
                .arg(live_visualization_frequency)
                .arg(cinema_tracks)
                .arg(filename)
                .arg(cinema_arrays)
                .arg(write_start)
                .arg(make_cinema_table)
                .arg(root_directory)
                .arg(request_specific_arrays)
                .arg(force_first_output);

    // ensure Python in initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(command.toLocal8Bit().data());
  }
#endif
}
