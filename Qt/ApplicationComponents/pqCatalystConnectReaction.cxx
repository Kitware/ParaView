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
