/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
