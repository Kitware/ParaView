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
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPython.h" // must be first

#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#include "pqCatalystExportReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidgetDialog.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <QWidget>

//-----------------------------------------------------------------------------
pqCatalystExportReaction::pqCatalystExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  parentObject->setEnabled(true);
#else
  parentObject->setEnabled(false);
  parentObject->setToolTip("Needs Python support");
  parentObject->setStatusTip("Needs Python support");
#endif
}

//-----------------------------------------------------------------------------
pqCatalystExportReaction::~pqCatalystExportReaction() = default;

//-----------------------------------------------------------------------------
QString pqCatalystExportReaction::exportScript()
{
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(), tr("Save Catalyst State:"),
    QString(), "Python file (*.py);;All files (*)");
  fileDialog.setObjectName("SaveCatalystStateFileDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (!fileDialog.exec())
  {
    return QString();
  }

  auto fname = fileDialog.getSelectedFiles()[0];
  return pqCatalystExportReaction::exportScript(fname) ? fname : QString();
}

//-----------------------------------------------------------------------------
bool pqCatalystExportReaction::exportScript(const QString& filename)
{
  if (filename.isEmpty())
  {
    return false;
  }

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  vtkNew<vtkSMParaViewPipelineController> controller;
  auto pxm = pqActiveObjects::instance().proxyManager();
  auto proxy = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("coprocessing", "CatalystOptions"));
  controller->InitializeProxy(proxy);
  pqProxyWidgetDialog dialog(proxy, pqCoreUtilities::mainWidget());
  dialog.setWindowTitle("Save Catalyst State Options");
  dialog.setSettingsKey("CatalystOptions");
  dialog.setEnableSearchBar(true);
  if (dialog.exec() != QDialog::Accepted)
  {
    return false;
  }

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject module(PyImport_ImportModule("paraview.detail.catalyst_export"));
  if (!module)
  {
    qCritical("Failed to import 'paraview.detail.catalyst_export'.");
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }
  vtkSmartPyObject name(PyString_FromString("save_catalyst_state"));
  vtkSmartPyObject pyfilename(PyString_FromString(filename.toLocal8Bit().data()));
  vtkSmartPyObject pyproxy(vtkPythonUtil::GetObjectFromPointer(proxy));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    module, name, pyfilename.GetPointer(), pyproxy.GetPointer(), nullptr));
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return false;
  }
  return true;
#else
  qCritical("Catalyst state cannot be exported since Python support not enabled in this build.");
  return false;
#endif
}
