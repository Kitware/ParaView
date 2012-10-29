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
#include "pqServer.h"
#include "vtkProcessModule.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

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
    // Make sure we are in multi-server mode
    vtkProcessModule::GetProcessModule()->MultipleSessionsSupportOn();

    bool user_ok = false;
    int portNumber = QInputDialog::getInt(
      pqCoreUtilities::mainWidget(),
      "Catalyst Server Port",
      "Enter the port number to accept connections \nfrom Catalyst on:",
      22222, 1024, 0x0fffffff, 1, &user_ok);
    if (!user_ok)
      {
      // user cancelled.
      return false;
      }

    pqLiveInsituVisualizationManager* mgr =
      new pqLiveInsituVisualizationManager(portNumber, server);
    QObject::connect(mgr, SIGNAL(catalystDisconnected()),
      this, SLOT(onCatalystDisconnected()));
    this->Managers[server] = mgr;
    this->updateEnableState();
    QMessageBox::information(pqCoreUtilities::mainWidget(),
      "Ready for Catalyst connections",
      QString("Accepting connections from Catalyst Co-Processor \n"
      "for live-coprocessing on port %1").arg(portNumber));
    return true;
    }

  qWarning("A Catalyst connection has already been established.");
  return false;
}


//-----------------------------------------------------------------------------
void pqCatalystConnectReaction::onCatalystDisconnected()
{
  pqLiveInsituVisualizationManager* mgr =
    qobject_cast<pqLiveInsituVisualizationManager*>(this->sender());
  if (!mgr)
    {
    return;
    }

  QMessageBox::information(pqCoreUtilities::mainWidget(),
    "Catalyst Disconnected",
    "Connection to Catalyst Co-Processor has been terminated involuntarily. "
    "This implies either a communication error, or that the "
    "Catalyst co-processor has terminated. "
    "The Catalyst session will now be cleaned up. "
    "You can start a new one if you want to monitor for additional Catalyst "
    "connection requests.");

  mgr->deleteLater();

  // Remove the mgr from the map, so that we can allow the user to connect to
  // another Catalyst session, if he wants.
  for (ManagersType::iterator iter = this->Managers.begin();
    iter != this->Managers.end(); ++iter)
    {
    if (iter.value() == mgr)
      {
      this->Managers.erase(iter);
      break;
      }
    }

  // Depending upon which server was currently active, the activeServerChanged()
  // signal may never be fired. If that happens, we won't get any opportunity to
  // change update the action enable state. So do it now.
  this->updateEnableState();
}
