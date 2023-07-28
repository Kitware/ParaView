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
bool pqCatalystConnectReaction::connect()
{
  pqLiveInsituManager* cs = pqLiveInsituManager::instance();
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqLiveInsituVisualizationManager* mgr = cs->connect(server);
  if (mgr)
  {
    this->updateEnableState();
    QObject::connect(mgr, SIGNAL(insituDisconnected()), this, SLOT(updateEnableState()));
  }
  return mgr;
}

//-----------------------------------------------------------------------------
void pqCatalystConnectReaction::updateEnableState()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server && !pqLiveInsituManager::isInsituServer(server) &&
    !server->session()->IsMultiClients() &&
    !pqLiveInsituManager::instance()->isDisplayServer(server))
  {
    this->parentAction()->setEnabled(true);
  }
  else
  {
    this->parentAction()->setEnabled(false);
  }
}
