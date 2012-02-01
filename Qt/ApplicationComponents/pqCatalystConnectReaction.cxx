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
#include "pqLiveInsituVisualizationManager.h"
#include "pqOptions.h"
#include "pqServer.h"
#include "vtkSMSession.h"

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::pqCatalystConnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::~pqCatalystConnectReaction()
{
}

//-----------------------------------------------------------------------------
void pqCatalystConnectReaction::updateEnableState()
{
  if (!pqApplicationCore::instance()->getOptions()->GetMultiServerMode())
    {
    this->parentAction()->setEnabled(false);
    return;
    }

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server && 
    server->getResource().scheme() != "catalyst" &&
    !server->session()->IsMultiClients() &&
    !this->Managers.contains(server))
    {
    this->parentAction()->setEnabled(true); 
    }
  else
    {
    this->parentAction()->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
bool pqCatalystConnectReaction::connect()
{
  pqServer* server = pqActiveObjects::instance().activeServer();

  if (server != NULL || !this->Managers.contains(server))
    {
    pqLiveInsituVisualizationManager* mgr =
      new pqLiveInsituVisualizationManager(22222, server);
    this->Managers[server] = mgr;
    this->updateEnableState();
    return true;
    }

  return false;
}
