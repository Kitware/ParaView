/*=========================================================================

   Program: ParaView
   Module:    pqServer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqServer.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOptions.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimeKeeper.h"
#include "pqView.h"
#include "vtkClientServerStream.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMapper.h" // Needed for VTK_RESOLVE_SHIFT_ZBUFFER
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMMessage.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkToolkits.h"

// Qt includes.
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QTimer>
#include <QtDebug>

class pqServer::pqInternals
{
public:
  QPointer<pqTimeKeeper> TimeKeeper;
  // Used to send an heart beat message to the server to avoid
  // inactivity timeouts.
  QTimer HeartbeatTimer;

  QTimer ServerLifeTimeTimer;

  // remaining time in minutes
  int RemainingLifeTime{ -1 };

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkWeakPointer<vtkSMCollaborationManager> CollaborationCommunicator;
};
/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

//-----------------------------------------------------------------------------
pqServer::pqServer(vtkIdType connectionID, vtkPVOptions* options, QObject* _parent)
  : pqServerManagerModelItem(_parent)
{
  this->Internals = new pqInternals;

  this->ConnectionID = connectionID;
  this->Options = options;
  this->Session =
    vtkSMSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession(connectionID));

  QObject::connect(&this->Internals->ServerLifeTimeTimer, &QTimer::timeout, this,
    &pqServer::updateRemainingLifeTime);

  vtkPVServerInformation* serverInfo = this->getServerInformation();
  const int timeout = (this->isRemote() && serverInfo && serverInfo->GetTimeout() > 0)
    ? serverInfo->GetTimeout()
    : -1;
  this->setRemainingLifeTime(timeout);

  QObject::connect(&this->Internals->HeartbeatTimer, SIGNAL(timeout()), this, SLOT(heartBeat()));

  this->setHeartBeatTimeout(pqServer::getHeartBeatTimeoutSetting());

  // Setup idle Timer for collaboration in order to get server notification
  this->IdleCollaborationTimer.setInterval(100);
  this->IdleCollaborationTimer.setSingleShot(true);
  QObject::connect(
    &this->IdleCollaborationTimer, SIGNAL(timeout()), this, SLOT(processServerNotification()));

  // Monitor server crash for better error management
  this->Internals->VTKConnect->Connect(this->Session, vtkPVSessionBase::ConnectionLost, this,
    SLOT(onConnectionLost(vtkObject*, ulong, void*, void*)));

  // In case of Multi-clients connection, the client has to listen
  // server notification so collaboration could happen
  if (this->session()->IsMultiClients())
  {
    vtkSMSessionClient* currentSession = vtkSMSessionClient::SafeDownCast(this->session());
    if (currentSession)
    {
      // Initialise the CollaborationManager to listen server notification
      this->Internals->CollaborationCommunicator = currentSession->GetCollaborationManager();
      this->Internals->VTKConnect->Connect(currentSession->GetCollaborationManager(),
        vtkCommand::AnyEvent, this,
        SLOT(onCollaborationCommunication(vtkObject*, ulong, void*, void*)));
    }
    this->setMonitorServerNotifications(true);
  }
}

//-----------------------------------------------------------------------------
pqServer::~pqServer()
{
  // Close the connection.
  /* It's not good to disonnect when the object is destroyed, since
     the connection was not created when the object was created.
     Let who ever created the connection, close it.
     */
  /*
  if (this->ConnectionID != vtkProcessModuleConnectionManager::GetNullConnectionID()
    && this->ConnectionID
    != vtkProcessModuleConnectionManager::GetSelfConnectionID())
    {
    vtkProcessModule::GetProcessModule()->Disconnect(this->ConnectionID);
    }
    */
  this->ConnectionID = 0;
  this->Session = NULL;
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqServer::setMonitorServerNotifications(bool val)
{
  if (val)
  {
    this->IdleCollaborationTimer.start();
  }
  else
  {
    this->IdleCollaborationTimer.stop();
  }
}

//-----------------------------------------------------------------------------
void pqServer::setRemainingLifeTime(int value)
{
  auto& internals = (*this->Internals);
  if (internals.RemainingLifeTime != value)
  {
    internals.RemainingLifeTime = value;
    if (value > 0 && internals.ServerLifeTimeTimer.isActive() == false)
    {
      internals.ServerLifeTimeTimer.start(60000); // trigger signal every minute
    }
    else if (value <= 0)
    {
      internals.ServerLifeTimeTimer.stop();
    }
    // since RemainingLifeTime is used it labelling the server, fire nameChanged
    // so pipeline browser can accurately indicate it.
    emit this->nameChanged(this);
  }
}

//-----------------------------------------------------------------------------
int pqServer::getRemainingLifeTime() const
{
  return this->Internals->RemainingLifeTime;
}

//-----------------------------------------------------------------------------
pqTimeKeeper* pqServer::getTimeKeeper() const
{
  if (!this->Internals->TimeKeeper)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    vtkSMProxy* proxy = controller->FindTimeKeeper(this->session());
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    this->Internals->TimeKeeper = smmodel->findItem<pqTimeKeeper*>(proxy);
  }

  return this->Internals->TimeKeeper;
}

//-----------------------------------------------------------------------------
vtkSMSession* pqServer::session() const
{
  return this->Session.GetPointer();
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel* pqServer::activeSourcesSelectionModel() const
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  return pxm->GetSelectionModel("ActiveSources");
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel* pqServer::activeViewSelectionModel() const
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  return pxm->GetSelectionModel("ActiveView");
}

//-----------------------------------------------------------------------------
const pqServerResource& pqServer::getResource()
{
  return this->Resource;
}

//-----------------------------------------------------------------------------
vtkIdType pqServer::GetConnectionID() const
{
  return this->ConnectionID;
}

//-----------------------------------------------------------------------------
int pqServer::getNumberOfPartitions()
{
  return this->Session->GetNumberOfProcesses(
    vtkPVSession::DATA_SERVER | vtkPVSession::RENDER_SERVER);
}

//-----------------------------------------------------------------------------
bool pqServer::isRemote() const
{
  return this->Session->IsA("vtkSMSessionClient");
}

//-----------------------------------------------------------------------------
bool pqServer::isMaster() const
{
  if (this->session()->IsMultiClients())
  {
    vtkSMSessionClient* currentSession = vtkSMSessionClient::SafeDownCast(this->session());
    if (currentSession)
    {
      return currentSession->GetCollaborationManager()->IsMaster();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool pqServer::isRenderServerSeparate()
{
  if (this->isRemote())
  {
    return this->Session->GetController(vtkPVSession::DATA_SERVER_ROOT) !=
      this->Session->GetController(vtkPVSession::RENDER_SERVER_ROOT);
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqServer::setResource(const pqServerResource& server_resource)
{
  this->Resource = server_resource;
  emit this->nameChanged(this);
}

//-----------------------------------------------------------------------------
vtkPVOptions* pqServer::getOptions() const
{
  return this->Options;
}

//-----------------------------------------------------------------------------
vtkPVServerInformation* pqServer::getServerInformation() const
{
  return this->Session->GetServerInformation();
}

//-----------------------------------------------------------------------------
bool pqServer::isProgressPending() const
{
  return (this->Session && this->Session->GetPendingProgress());
}

//-----------------------------------------------------------------------------
void pqServer::setHeartBeatTimeout(int msec)
{
  // no need to set heart beats if not a remote connection.
  if (this->isRemote())
  {
    if (msec <= 0)
    {
      this->Internals->HeartbeatTimer.stop();
    }
    else
    {
      this->heartBeat();
      this->Internals->HeartbeatTimer.start(msec);
    }
  }
}

//-----------------------------------------------------------------------------
void pqServer::heartBeat()
{
  // Send random stream to all processes to produce some traffic and prevent
  // automatic disconnection
  if (this->Session && !this->Session->GetPendingProgress())
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << "HeartBeat" << vtkClientServerStream::End;
    this->Session->ExecuteStream(vtkPVSession::SERVERS, stream, true);
  }
}

//-----------------------------------------------------------------------------
void pqServer::updateRemainingLifeTime()
{
  if (this->isRemote() && this->Internals->RemainingLifeTime > 0)
  {
    this->Internals->RemainingLifeTime--;
    if (this->Internals->RemainingLifeTime == 5)
    {
      emit fiveMinuteTimeoutWarning();
    }
    else if (this->Internals->RemainingLifeTime == 1)
    {
      emit finalTimeoutWarning();
    }

    emit this->nameChanged(this);
  }
}

//-----------------------------------------------------------------------------
const char* pqServer::HEARBEAT_TIME_SETTING_KEY()
{
  return "/server/HeartBeatTime";
}

//-----------------------------------------------------------------------------
void pqServer::setHeartBeatTimeoutSetting(int msec)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
  {
    settings->setValue(pqServer::HEARBEAT_TIME_SETTING_KEY(), QVariant(msec));
  }

  // update all current servers.
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  foreach (pqServer* server, servers)
  {
    server->setHeartBeatTimeout(msec);
  }
}

//-----------------------------------------------------------------------------
int pqServer::getHeartBeatTimeoutSetting()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings && settings->contains(pqServer::HEARBEAT_TIME_SETTING_KEY()))
  {
    bool ok;
    int timeout = settings->value(pqServer::HEARBEAT_TIME_SETTING_KEY()).toInt(&ok);
    if (ok)
    {
      return timeout;
    }
  }
  return 1 * 60 * 1000; // 1 minutes.
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqServer::proxyManager() const
{
  return this->Session->GetSessionProxyManager();
}

//-----------------------------------------------------------------------------
void pqServer::processServerNotification()
{
  vtkSMSessionClient* sessionClient = vtkSMSessionClient::SafeDownCast(this->Session);
  if ((sessionClient && !sessionClient->IsNotBusy()) || this->isProgressPending())
  {
    // try again later.
    this->IdleCollaborationTimer.start();
    return;
  }

  // process all server-notification events.
  vtkNetworkAccessManager* nam = vtkProcessModule::GetProcessModule()->GetNetworkAccessManager();
  while (nam->ProcessEvents(1) == 1)
  {
  }

  foreach (pqView* view, pqApplicationCore::instance()->findChildren<pqView*>())
  {
    vtkSMViewProxy* viewProxy = view->getViewProxy();
    if (viewProxy && viewProxy->HasDirtyRepresentation())
    {
      view->render();
    }
  }
  this->IdleCollaborationTimer.start();
}

//-----------------------------------------------------------------------------
void pqServer::onCollaborationCommunication(
  vtkObject* vtkNotUsed(src), unsigned long event_, void* vtkNotUsed(method), void* data)
{
  int userId;
  QString userName;
  switch (event_)
  {
    case vtkSMCollaborationManager::UpdateUserName:
      userId = *reinterpret_cast<int*>(data);
      userName = this->Internals->CollaborationCommunicator->GetUserLabel(userId);
      emit triggeredUserName(userId, userName);
      break;
    case vtkSMCollaborationManager::UpdateUserList:
      emit triggeredUserListChanged();
      break;
    case vtkSMCollaborationManager::UpdateMasterUser:
      userId = *reinterpret_cast<int*>(data);
      emit triggeredMasterUser(userId);
      break;
    case vtkSMCollaborationManager::FollowUserCamera:
      userId = *reinterpret_cast<int*>(data);
      emit triggerFollowCamera(userId);
      break;
    case vtkSMCollaborationManager::CollaborationNotification:
      vtkSMMessage* msg = reinterpret_cast<vtkSMMessage*>(data);
      emit sentFromOtherClient(this, msg);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqServer::onConnectionLost(vtkObject*, unsigned long, void*, void*)
{
  emit serverSideDisconnected();
}
//-----------------------------------------------------------------------------
void pqServer::sendToOtherClients(vtkSMMessage* msg)
{
  if (this->Internals->CollaborationCommunicator)
  {
    this->Internals->CollaborationCommunicator->SendToOtherClients(msg);
  }
}
//-----------------------------------------------------------------------------
bool pqServer::isProcessingPending() const
{
  // check with the network access manager if there are any messages to receive
  // from the server.
  bool retVal =
    vtkProcessModule::GetProcessModule()->GetNetworkAccessManager()->GetNetworkEventsAvailable();
  return retVal;
}
