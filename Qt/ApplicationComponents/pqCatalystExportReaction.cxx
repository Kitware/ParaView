// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  parentObject->setToolTip(tr("Needs Python support"));
  parentObject->setStatusTip(tr("Needs Python support"));
#endif
}

//-----------------------------------------------------------------------------
pqCatalystExportReaction::~pqCatalystExportReaction() = default;

//-----------------------------------------------------------------------------
QString pqCatalystExportReaction::exportScript()
{
  QString fileExt;
#if VTK_MODULE_ENABLE_ParaView_pqPython
  fileExt += tr("Catalyst state file") + QString(" (*.py);;");
#endif
  fileExt += tr("All files") + QString(" (*)");
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Save Catalyst State:"),
    QString(), fileExt, false, false);
  fileDialog.setObjectName("SaveCatalystStateFileDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (!fileDialog.exec())
  {
    return QString();
  }

  auto filename = fileDialog.getSelectedFiles()[0];
  auto location = fileDialog.getSelectedLocation();
  return pqCatalystExportReaction::exportScript(filename, location) ? filename : QString();
}

//-----------------------------------------------------------------------------
bool pqCatalystExportReaction::exportScript(const QString& filename, vtkTypeUInt32 location)
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
  dialog.setWindowTitle(tr("Save Catalyst State Options"));
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
  vtkSmartPyObject name(PyUnicode_FromString("save_catalyst_state"));
  vtkSmartPyObject pyfilename(PyUnicode_FromString(filename.toUtf8().data()));
  vtkSmartPyObject pyproxy(vtkPythonUtil::GetObjectFromPointer(proxy));
  vtkSmartPyObject pylocation(PyLong_FromUnsignedLong(location));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    module, name, pyfilename.GetPointer(), pyproxy.GetPointer(), pylocation.GetPointer(), nullptr));
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return false;
  }
  return true;
#else
  Q_UNUSED(location);
  qCritical("Catalyst state cannot be exported since Python support not enabled in this build.");
  return false;
#endif
}
