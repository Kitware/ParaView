// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServer.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
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

  // Server side timeout command and time interval between command invocations in seconds
  std::string TimeoutCommand;
  int TimeoutCommandInterval{ 60 };

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkWeakPointer<vtkSMCollaborationManager> CollaborationCommunicator;
};
/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

//-----------------------------------------------------------------------------
pqServer::pqServer(vtkIdType connectionID, QObject* _parent)
  : pqServerManagerModelItem(_parent)
{
  this->Internals = new pqInternals;

  this->ConnectionID = connectionID;
  this->Session =
    vtkSMSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession(connectionID));

  vtkPVServerInformation* serverInfo = this->getServerInformation();

  if (this->isRemote() && serverInfo)
  {
    this->Internals->TimeoutCommand = serverInfo->GetTimeoutCommand();
    this->Internals->TimeoutCommandInterval = serverInfo->GetTimeoutCommandInterval();
    int val = serverInfo->GetTimeout();
    this->Internals->RemainingLifeTime = val > 0 ? val : -1;
  }

  if (!this->Internals->TimeoutCommand.empty()) // Setup server side command proxy
  {
#if defined(_WIN32)
    // On Windows, we need to specify to start a new instance of the command
    // interpreter (cmd.exe) and indicate to terminate when done with the command (/c)
    this->Internals->TimeoutCommand.insert(0, "cmd.exe /c ");
#endif
    vtkSMSessionProxyManager* pxm = proxyManager();
    this->ExecutableRunnerProxy.TakeReference(pxm->NewProxy("misc", "ExecutableRunner"));
    vtkSMPropertyHelper(this->ExecutableRunnerProxy, "Command")
      .Set(this->Internals->TimeoutCommand.c_str());
    vtkSMPropertyHelper(this->ExecutableRunnerProxy, "Timeout").Set(0.3);
    this->ExecutableRunnerProxy->UpdateVTKObjects();
    this->updateRemainingLifeTime();
  }

  QObject::connect(&this->Internals->ServerLifeTimeTimer, &QTimer::timeout, this,
    &pqServer::updateRemainingLifeTime);

  this->Internals->ServerLifeTimeTimer.setInterval(1000 * this->Internals->TimeoutCommandInterval);
  this->Internals->ServerLifeTimeTimer.start();

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
  this->Session = nullptr;
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
  if (this->Internals->RemainingLifeTime != value)
  {
    this->Internals->RemainingLifeTime = value;
    Q_EMIT this->nameChanged(this);
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
  Q_EMIT this->nameChanged(this);
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
  if (this->isRemote())
  {
    if (!this->Internals->TimeoutCommand.empty()) // Update with command
    {
      // Launch timeout command and retrieve the result
      this->ExecutableRunnerProxy->InvokeCommand("Execute");
      this->ExecutableRunnerProxy->UpdatePropertyInformation();
      int returnValue = vtkSMPropertyHelper(this->ExecutableRunnerProxy, "ReturnValue").GetAsInt();

      if (returnValue == 0)
      {
        this->Internals->RemainingLifeTime =
          vtkSMPropertyHelper(this->ExecutableRunnerProxy, "StdOut").GetAsInt();
      }
      else if (returnValue > 0)
      {
        vtkGenericWarningMacro("Error when executing the server side timeout command : "
          << vtkSMPropertyHelper(this->ExecutableRunnerProxy, "StdErr").GetAsString());
      }
      else
      {
        vtkGenericWarningMacro("Unable to run the server side timeout command.");
      }
    }
    else if (this->Internals->RemainingLifeTime > 0) // Local update
    {
      this->Internals->RemainingLifeTime--;
    }

    // Time warnings
    if (this->Internals->RemainingLifeTime == 5)
    {
      Q_EMIT fiveMinuteTimeoutWarning();
    }
    else if (this->Internals->RemainingLifeTime == 1)
    {
      Q_EMIT finalTimeoutWarning();
    }
    Q_EMIT this->nameChanged(this);
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
  Q_FOREACH (pqServer* server, servers)
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

  Q_FOREACH (pqView* view, pqApplicationCore::instance()->findChildren<pqView*>())
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
      Q_EMIT triggeredUserName(userId, userName);
      break;
    case vtkSMCollaborationManager::UpdateUserList:
      Q_EMIT triggeredUserListChanged();
      break;
    case vtkSMCollaborationManager::UpdateMasterUser:
      userId = *reinterpret_cast<int*>(data);
      Q_EMIT triggeredMasterUser(userId);
      break;
    case vtkSMCollaborationManager::FollowUserCamera:
      userId = *reinterpret_cast<int*>(data);
      Q_EMIT triggerFollowCamera(userId);
      break;
    case vtkSMCollaborationManager::CollaborationNotification:
      vtkSMMessage* msg = reinterpret_cast<vtkSMMessage*>(data);
      Q_EMIT sentFromOtherClient(this, msg);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqServer::onConnectionLost(vtkObject*, unsigned long, void*, void*)
{
  Q_EMIT serverSideDisconnected();
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
