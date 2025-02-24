// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLoadStateReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPVApplicationCore.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkPVXMLParser.h"
#include "vtkSMLoadStateOptionsProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtksys/SystemTools.hxx"

#include <QFileInfo>

//-----------------------------------------------------------------------------
pqLoadStateReaction::pqLoadStateReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != nullptr);
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::loadState(
  const QString& filename, bool dialogBlocked, pqServer* server, vtkTypeUInt32 location)
{
  SCOPED_UNDO_EXCLUDE();

  if (server == nullptr)
  {
    server = pqActiveObjects::instance().activeServer();
  }

  if (!server)
  {
    return;
  }

  if (filename.endsWith(".pvsm") || filename.endsWith(".png"))
  {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    vtkSmartPointer<vtkSMProxy> aproxy;
    aproxy.TakeReference(pxm->NewProxy("options", "LoadStateOptions"));
    vtkSMLoadStateOptionsProxy* proxy = vtkSMLoadStateOptionsProxy::SafeDownCast(aproxy);
    vtkSMPropertyHelper(proxy, "DataDirectory")
      .Set(vtksys::SystemTools::GetParentDirectory(filename.toUtf8().toStdString()).c_str());

    Q_EMIT pqPVApplicationCore::instance()->aboutToReadState(filename);

    if (proxy && proxy->PrepareToLoad(filename.toUtf8().data(), location))
    {
      vtkNew<vtkSMParaViewPipelineController> controller;
      controller->InitializeProxy(proxy);

      if (proxy->HasDataFiles() && !dialogBlocked)
      {
        pqProxyWidgetDialog dialog(proxy);
        dialog.setWindowTitle(tr("Load State Options"));
        dialog.setObjectName("LoadStateOptionsDialog");
        dialog.setApplyChangesImmediately(true);
        if (dialog.exec() != QDialog::Accepted)
        {
          return;
        }
      }
      pqPVApplicationCore::instance()->clearViewsForLoadingState(server);
      pqPVApplicationCore::instance()->setLoadingState(true);
      if (proxy->Load())
      {
        pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
          server, filename, location);
      }
      pqPVApplicationCore::instance()->setLoadingState(false);

      // This is needed since XML state currently does not save active view.
      // Check for an already active view that could have been set by a custom behavior.
      if (!pqActiveObjects::instance().activeView())
      {
        pqLoadStateReaction::activateView();
      }
    }
  }
  else
  { // python file
#if VTK_MODULE_ENABLE_ParaView_pqPython
    // pqPVApplicationCore::loadStateFromPythonFile already emits aboutToReadState
    pqPVApplicationCore::instance()->loadStateFromPythonFile(filename, server, location);
    pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
      server, filename, location);
#else
    qWarning("ParaView was not built with Python support so it cannot open a python file");
#endif
  }
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::loadState()
{
  bool pythonAvailable = false;
#if VTK_MODULE_ENABLE_ParaView_pqPython
  pythonAvailable = true;
#endif

  QString fileExt =
    pqApplicationCore::instance()->getDefaultSaveStateFileFormatQString(pythonAvailable, true);

  auto server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Load State File"), QString(), fileExt, false, false);
  fileDialog.setObjectName("FileLoadServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    const QString selectedFile = fileDialog.getSelectedFiles()[0];
    pqLoadStateReaction::loadState(selectedFile, false, server, fileDialog.getSelectedLocation());
  }
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::activateView()
{
  auto server = pqActiveObjects::instance().activeServer();
  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  auto views = smmodel->findItems<pqView*>(server);
  if (!views.empty())
  {
    pqActiveObjects::instance().setActiveView(views[0]);
  }
}
