// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServerConnectReaction.h"

#include "vtkProcessModule.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerConfigurationCollection.h"
#include "pqServerConnectDialog.h"
#include "pqServerLauncher.h"
#include "pqServerManagerModel.h"

#include <QMessageBox>
#include <QScopedPointer>
#include <QtDebug>
//-----------------------------------------------------------------------------
pqServerConnectReaction::pqServerConnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // needed to disable server connection while an animation is playing
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay), this,
    &pqServerConnectReaction::updateEnableState);
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay), this,
    &pqServerConnectReaction::updateEnableState);
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::connectToServerWithWarning()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  pqServer* server = pqActiveObjects::instance().activeServer();

  if (!vtkProcessModule::GetProcessModule()->GetMultipleSessionsSupport() &&
    !smmodel->findItems<pqPipelineSource*>(server).empty())
  {
    int ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Disconnect from current server?"),
        QString("%1\n\n%2")
          .arg(tr("Before connecting to a new server, the current connection will be closed and "
                  "the state will be discarded. "))
          .arg(tr("Are you sure you want to continue?")),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return;
    }
  }

  pqServerConnectReaction::connectToServer();
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::connectToServer()
{
  pqServerConnectDialog dialog(pqCoreUtilities::mainWidget());
  if (dialog.exec() == QDialog::Accepted)
  {
    pqServerConnectReaction::connectToServerUsingConfiguration(dialog.configurationToConnect());
  }
}

//-----------------------------------------------------------------------------
bool pqServerConnectReaction::connectToServerUsingConfigurationName(
  const char* config_name, bool showConnectionDialog)
{
  const pqServerConfiguration* config =
    pqApplicationCore::instance()->serverConfigurations().configuration(config_name);
  if (config)
  {
    return pqServerConnectReaction::connectToServerUsingConfiguration(
      *config, showConnectionDialog);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqServerConnectReaction::connectToServer(
  const pqServerResource& resource, bool showConnectionDialog)
{
  pqServerConfiguration config;
  config.setResource(resource);
  if (!resource.serverName().isEmpty())
  {
    config.setName(resource.serverName());
  }
  return pqServerConnectReaction::connectToServerUsingConfiguration(config, showConnectionDialog);
}

//-----------------------------------------------------------------------------
bool pqServerConnectReaction::connectToServerUsingConfiguration(
  const pqServerConfiguration& config, bool showConnectionDialog)
{
  QScopedPointer<pqServerLauncher> launcher(pqServerLauncher::newInstance(config));
  return launcher->connectToServer(showConnectionDialog);
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::updateEnableState()
{
  if (pqPVApplicationCore::instance()->animationManager()->animationPlaying())
  {
    this->parentAction()->setEnabled(false);
  }
  else
  {
    this->parentAction()->setEnabled(true);
  }
}
