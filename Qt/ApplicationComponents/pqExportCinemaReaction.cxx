/*=========================================================================

   Program: ParaView
   Module:    pqExportCinemaReaction.cxx

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
#include "pqExportCinemaReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidget.h"
#include "vtkNew.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_ENABLE_PYTHON
#ifdef PARAVIEW_ENABLE_PYTHON
# include "vtkPythonInterpreter.h"
#endif
#include "vtkSmartPointer.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMViewExportHelper.h"
#include "vtkSMViewProxy.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QtDebug>
#include <QtGlobal>
#include <QToolButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqExportCinemaReaction::pqExportCinemaReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqExportCinemaReaction::updateEnableState()
{
  // this results in firing of exportable(bool) signal which updates the
  // QAction's state.
  bool enabled = false;
  if (pqView* view = pqActiveObjects::instance().activeView())
    {
    vtkSMViewProxy* viewProxy = view->getViewProxy();

    vtkNew<vtkSMViewExportHelper> helper;
    enabled = (helper->GetSupportedFileTypes(viewProxy).size() > 0);
    }
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqExportCinemaReaction::exportActiveView()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view) { return ;}

  pqFileDialog folder_dialog(NULL, pqCoreUtilities::mainWidget(),
      tr("Export Cinema:"), QString(), "");
  folder_dialog.setObjectName("CinemaExportDialog");
  folder_dialog.setFileMode(pqFileDialog::Directory);
  if (folder_dialog.exec() == QDialog::Accepted &&
      folder_dialog.getSelectedFiles().size() > 0)
    {
    QString filename = folder_dialog.getSelectedFiles().first();
    //deal with MSWindows
    //there dialog returns the last component with just one slash
    int lastSlash = filename.lastIndexOf("\\");
    if (lastSlash!=-1)
      {
      if (filename[lastSlash-1] != QString("\\")[0])
        {
        filename.insert(lastSlash, "\\");
        }
      }
    //std::cerr << "Selected path: " << qPrintable(filename) << std::endl;

    std::string path;
    path += qPrintable(filename);

    std::string script;
    script += "import paraview\n";
    script += "ready=True\n";
    script += "try:\n";
    script += "    import paraview.simple\n";
    script += "    import paraview.cinemaIO.cinema_store\n";
    script += "    import paraview.cinemaIO.pv_introspect as pvi\n";
    script += "except ImportError:\n";
    script += "    paraview.print_error('Error: Cannot import cinema or a dependency')\n";
    script += "    ready=False\n";
    script += "if ready:\n";
    script += "    pvi.record(csname=\"";
    script += path.c_str();
    script += "\")\n";

    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(script.c_str());
    }
#endif
}
