// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCatalystConnectReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"

#include "vtkProcessModule.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::pqCatalystConnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::~pqCatalystConnectReaction() = default;

//-----------------------------------------------------------------------------
void pqCatalystConnectReaction::onTriggered()
{
  if (this->IsEstablished)
  {
    this->disconnect();
  }
  else
  {
    this->connect();
  }
}

//-----------------------------------------------------------------------------
bool pqCatalystConnectReaction::connect()
{
  pqLiveInsituManager* cs = pqLiveInsituManager::instance();
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqLiveInsituVisualizationManager* manager = cs->connect(server);

  if (manager)
  {
    this->updateEnableState();
    QObject::connect(manager, SIGNAL(insituDisconnected()), this, SLOT(updateEnableState()));

    return manager != nullptr;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqCatalystConnectReaction::disconnect()
{
  pqLiveInsituManager* inSituManager = pqLiveInsituManager::instance();
  if (!inSituManager)
  {
    return false;
  }

  pqServer* catalystServer = inSituManager->selectedInsituServer();
  if (!catalystServer)
  {
    return false;
  }

  pqLiveInsituVisualizationManager* mgr = pqLiveInsituManager::managerFromInsitu(catalystServer);
  if (!mgr)
  {
    return false;
  }

  inSituManager->closeConnection();
  this->updateEnableState();
  return true;
}

//-----------------------------------------------------------------------------
void pqCatalystConnectReaction::updateEnableState()
{
  pqServer* server = pqActiveObjects::instance().activeServer();

  // disable the action in collaboration mode
  bool inCollaborationMode = server && server->session()->IsMultiClients();
  if (inCollaborationMode)
  {
    this->parentAction()->setEnabled(false);
    return;
  }
  else
  {
    this->parentAction()->setEnabled(true);
  }

  if (server && !pqLiveInsituManager::isInsituServer(server) &&
    !pqLiveInsituManager::instance()->isDisplayServer(server))
  {
    this->parentAction()->setText(tr("Connect..."));
    this->IsEstablished = false;
  }
  else
  {
    this->parentAction()->setText(tr("Disconnect"));
    this->IsEstablished = true;
  }
}
