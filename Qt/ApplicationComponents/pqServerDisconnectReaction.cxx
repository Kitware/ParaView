// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServerDisconnectReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QMessageBox>

//-----------------------------------------------------------------------------
pqServerDisconnectReaction::pqServerDisconnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->UpdateTimer.setInterval(500);
  this->UpdateTimer.setSingleShot(true);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)),
    &this->UpdateTimer, SLOT(start()));
  QObject::connect(&this->UpdateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
  this->updateState();

  // needed to disable server disconnection while an animation is playing
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay), this,
    &pqServerDisconnectReaction::updateState);
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay), this,
    &pqServerDisconnectReaction::updateState);
}

//-----------------------------------------------------------------------------
bool pqServerDisconnectReaction::disconnectFromServerWithWarning()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  pqServer* server = pqActiveObjects::instance().activeServer();

  if (server && !smmodel->findItems<pqPipelineSource*>(server).empty())
  {
    int ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Disconnect from current server?"),
        tr("The current connection will be closed and\n"
           "the state will be discarded.\n\n"
           "Are you sure you want to continue?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return false;
    }
  }

  pqServerDisconnectReaction::disconnectFromServer();
  return true;
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::disconnectFromServer()
{
  auto& activeObjects = pqActiveObjects::instance();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer* server = activeObjects.activeServer();
  if (server)
  {
    core->getObjectBuilder()->removeServer(server);

    // set some other server active.
    if (activeObjects.activeServer() == nullptr)
    {
      auto smmodel = core->getServerManagerModel();
      auto allServers = smmodel->findItems<pqServer*>();
      if (!allServers.isEmpty())
      {
        pqActiveObjects::instance().setActiveServer(allServers[0]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::updateState()
{
  if (pqPVApplicationCore::instance()->animationManager()->animationPlaying())
  {
    this->parentAction()->setEnabled(false);
  }
  else
  {
    this->parentAction()->setEnabled(pqActiveObjects::instance().activeServer() != nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::onTriggered()
{
  this->parentAction()->setEnabled(false);
  if (!pqServerDisconnectReaction::disconnectFromServerWithWarning())
  {
    // re-enable the action since user cancelled the disconnect, otherwise the
    // action will be re-enabled when a new session, if any is created.
    this->parentAction()->setEnabled(true);
  }
}
