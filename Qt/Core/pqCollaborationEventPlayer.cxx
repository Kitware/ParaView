// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCollaborationEventPlayer.h"

#include "pqApplicationCore.h"
#include "pqCollaborationManager.h"
#include "pqEventDispatcher.h"
#include "vtkSMCollaborationManager.h"

#include <QEventLoop>

//-----------------------------------------------------------------------------
pqCollaborationEventPlayer::pqCollaborationEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqCollaborationEventPlayer::~pqCollaborationEventPlayer() = default;

//-----------------------------------------------------------------------------
void pqCollaborationEventPlayer::waitForMaster(int ms)
{
  pqCollaborationManager* mgr = qobject_cast<pqCollaborationManager*>(
    pqApplicationCore::instance()->manager("COLLABORATION_MANAGER"));
  // this process should just wait patiently until it becomes the master.
  while (mgr && mgr->activeCollaborationManager() && !mgr->activeCollaborationManager()->IsMaster())
  {
    pqEventDispatcher::processEventsAndWait(ms);
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationEventPlayer::waitForConnections(int num_connections)
{
  pqCollaborationManager* mgr = qobject_cast<pqCollaborationManager*>(
    pqApplicationCore::instance()->manager("COLLABORATION_MANAGER"));
  // this process should just wait patiently until it becomes the master.
  while (mgr && mgr->activeCollaborationManager() &&
    mgr->activeCollaborationManager()->GetNumberOfConnectedClients() < num_connections)
  {
    pqEventDispatcher::processEventsAndWait(500);
  }
  pqEventDispatcher::processEventsAndWait(1000);
}

//-----------------------------------------------------------------------------
void pqCollaborationEventPlayer::wait(int ms)
{
  pqEventDispatcher::processEventsAndWait(ms);
}

//-----------------------------------------------------------------------------
bool pqCollaborationEventPlayer::playEvent(
  QObject*, const QString& command, const QString& vtkNotUsed(arguments), bool& vtkNotUsed(error))
{
  if (command == "waitForMaster")
  {
    pqCollaborationEventPlayer::waitForMaster();
    return true;
  }
  else if (command == "waitForConnections")
  {
    pqCollaborationEventPlayer::waitForConnections(2);
    return true;
  }
  else if (command == "wait")
  {
    pqCollaborationEventPlayer::wait(1000);
    return true;
  }
  return false;
}
