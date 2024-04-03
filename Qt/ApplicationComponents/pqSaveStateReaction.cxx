// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveStateReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QFile>
#include <QTextStream>
#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqSaveStateReaction::pqSaveStateReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // save state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != nullptr);
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState()
{
  return pqSaveStateReaction::saveState(pqActiveObjects::instance().activeServer());
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState(pqServer* server)
{
  QString fileExt = tr("ParaView state file") + QString(" (*.pvsm);;");
#if VTK_MODULE_ENABLE_ParaView_pqPython
  fileExt += tr("Python state file") + QString(" (*.py);;");
#endif
  fileExt += tr("All Files") + QString(" (*)");

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Save State File"), QString(), fileExt, false, false);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() == QDialog::Accepted)
  {
    const QString selectedFile = fileDialog.getSelectedFiles()[0];
    if (selectedFile.endsWith(".py"))
    {
      pqSaveStateReaction::savePythonState(selectedFile, fileDialog.getSelectedLocation());
    }
    else
    {
      pqSaveStateReaction::saveState(selectedFile, fileDialog.getSelectedLocation());
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState(const QString& filename, vtkTypeUInt32 location)
{
  if (!pqApplicationCore::instance()->saveState(filename, location))
  {
    qCritical() << "Failed to save " << filename;
    return false;
  }
  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename, location);
  return true;
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::savePythonState(const QString& filename, vtkTypeUInt32 location)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  assert(pxm);

  vtkSmartPointer<vtkSMProxy> options;
  options.TakeReference(pxm->NewProxy("pythontracing", "PythonStateOptions"));
  if (options.GetPointer() == nullptr)
  {
    return false;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(options);

  pqProxyWidgetDialog dialog(options);
  dialog.setWindowTitle(tr("Python State Options"));
  dialog.setObjectName("StateOptionsDialog");
  dialog.setApplyChangesImmediately(true);
  if (dialog.exec() != QDialog::Accepted)
  {
    return false;
  }

  const std::string state = vtkSMTrace::GetState(options);
  if (state.empty())
  {
    qWarning("Empty state generated.");
    return false;
  }

  Q_EMIT pqApplicationCore::instance()->aboutToWriteState(filename);

  if (!pxm->SaveString(state.c_str(), filename.toStdString().c_str(), location))
  {
    qCritical() << tr("Failed to save state in ") << filename;
    return false;
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename, location);
  return true;
#else
  Q_UNUSED(location);
  qCritical() << "Failed to save '" << filename
              << "' since Python support in not enabled in this build.";
  return false;
#endif
}
