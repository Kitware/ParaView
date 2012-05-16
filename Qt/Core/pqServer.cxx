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
#include "vtkMapper.h"                 // Needed for VTK_RESOLVE_SHIFT_ZBUFFER
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
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
#include <QDir>
#include <QCoreApplication>
#include <QtDebug>
#include <QStringList>
#include <QTimer>

class pqServer::pqInternals
{
public:
  QPointer<pqTimeKeeper> TimeKeeper;
  // Used to send an heart beat message to the server to avoid 
  // inactivity timeouts.
  QTimer HeartbeatTimer;

  int IdleServerMessageCounter;

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkWeakPointer<vtkSMCollaborationManager> CollaborationCommunicator;
};
/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

//-----------------------------------------------------------------------------
pqServer::pqServer(vtkIdType connectionID, vtkPVOptions* options, QObject* _parent) :
  pqServerManagerModelItem(_parent)
{
  this->Internals = new pqInternals;

  this->ConnectionID = connectionID;
  this->Options = options;
  this->Session = vtkSMSession::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetSession(connectionID));

  vtkPVServerInformation* serverInfo = this->getServerInformation();
  if (this->isRemote() && serverInfo && serverInfo->GetTimeout() > 0)
    {
    int timeout = serverInfo->GetTimeout();
    if (timeout > 5)
      {
      // 5 minute warning is shown only if timeout > 5.
      QTimer::singleShot(
        (timeout-5)*60*1000, this, SIGNAL(fiveMinuteTimeoutWarning()));
      }

    // 1 minute warning.
    QTimer::singleShot(
        (timeout-1)*60*1000, this, SIGNAL(finalTimeoutWarning()));
    }

  QObject::connect(&this->Internals->HeartbeatTimer, SIGNAL(timeout()),
    this, SLOT(heartBeat()));

  this->setHeartBeatTimeout(pqServer::getHeartBeatTimeoutSetting());

  // Setup idle Timer for collaboration in order to get server notification
  this->IdleCollaborationTimer.setInterval(100);
  this->IdleCollaborationTimer.setSingleShot(true);
  QObject::connect(&this->IdleCollaborationTimer, SIGNAL(timeout()),
                   this, SLOT(processServerNotification()));
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
void pqServer::initialize()
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  // Update ProxyManager based on its remote state
  pxm->UpdateFromRemote();

  // setup the active-view and active-sources selection models.
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveSources");
  if (selmodel == NULL)
    {
    selmodel = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveSources", selmodel);
    selmodel->FastDelete();
    }
  this->ActiveSources = selmodel;

  selmodel = pxm->GetSelectionModel("ActiveView");
  if (selmodel == NULL)
    {
    selmodel = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveView", selmodel);
    selmodel->FastDelete();
    }
  this->ActiveView = selmodel;


  // Setup the Connection TimeKeeper.
  // Currently, we are keeping seperate times per connection. Once we start
  // supporting multiple connections, we may want to the link the
  // connection times together.
  this->createTimeKeeper();

  // Create the GlobalMapperPropertiesProxy.
  vtkSMProxy* proxy = pxm->GetProxy("temp_prototypes", "GlobalMapperProperties");
  if(proxy == NULL)
    {
    proxy = pxm->NewProxy("misc", "GlobalMapperProperties");
    proxy->UpdateVTKObjects();
    pxm->RegisterProxy("temp_prototypes", "GlobalMapperProperties", proxy);
    proxy->FastDelete();
    }
  this->GlobalMapperPropertiesProxy = proxy;
  this->updateGlobalMapperProperties();

  // Create Strict Load Balancing Proxy
  pqSettings* settings = pqApplicationCore::instance()->settings();
  proxy = pxm->GetProxy("temp_prototypes", "StrictLoadBalancing");
  if (proxy == NULL)
    {
    proxy = pxm->NewProxy("misc", "StrictLoadBalancing");
    vtkSMPropertyHelper(proxy, "DisableExtentsTranslator").Set(
      settings->value("strictLoadBalancing", false).toBool());
    proxy->UpdateVTKObjects();

    pxm->RegisterProxy("temp_prototypes", "StrictLoadBalancing", proxy);
    proxy->FastDelete();
    }


  // In case of Multi-clients connection, the client has to listen
  // server notification so collaboration could happen
  if(this->session()->IsMultiClients())
    {
    this->IdleCollaborationTimer.start();
    vtkSMSessionClient* currentSession = vtkSMSessionClient::SafeDownCast(this->session());
    if(currentSession)
      {
      // Initialise the CollaborationManager to listen server notification
      this->Internals->CollaborationCommunicator = currentSession->GetCollaborationManager();
      this->Internals->VTKConnect->Connect(
          currentSession->GetCollaborationManager(),
          vtkCommand::AnyEvent,
          this,
          SLOT(onCollaborationCommunication(vtkObject*,ulong,void*,void*)));
      }
    }

  // Force a proper active SessionProxyManager once this one is fully initialized
  // after a collaborative update
  // As well as multi-server, force the newly created server connection to be the
  // active one
  if(vtkSMProxyManager::GetProxyManager()->GetActiveSession() == this->Session)
    {
    vtkSMProxyManager::GetProxyManager()->SetActiveSession((vtkSMSession*)NULL);
    vtkSMProxyManager::GetProxyManager()->SetActiveSession(this->Session);
    }
  else
    {
    vtkSMProxyManager::GetProxyManager()->SetActiveSession(this->Session);
    }

  // Allow Python shell to trigger a disconnect request. Make sure that request
  // get forwarded to pqServerDisconnectReaction
  pqCoreUtilities::connect(this->session(), vtkCommand::ExitEvent, this, SIGNAL(closeSessionRequest()));
}

//-----------------------------------------------------------------------------
pqTimeKeeper* pqServer::getTimeKeeper() const
{
  if(!this->Internals->TimeKeeper)
    {
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    vtkSMProxy* proxy = pxm->GetProxy("timekeeper", "TimeKeeper");
    pqServerManagerModel* smmodel =
        pqApplicationCore::instance()->getServerManagerModel();
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
  return this->ActiveSources;
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel* pqServer::activeViewSelectionModel() const
{
  return this->ActiveView;
}

//-----------------------------------------------------------------------------
void pqServer::createTimeKeeper()
{
  // Set Global Time keeper if needed.
  if(this->getTimeKeeper() == NULL)
    {
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    vtkSMProxy* proxy = pxm->NewProxy("misc","TimeKeeper");
    proxy->UpdateVTKObjects();
    pxm->RegisterProxy("timekeeper", "TimeKeeper", proxy);
    proxy->FastDelete();
    pqServerManagerModel* smmodel =
        pqApplicationCore::instance()->getServerManagerModel();
    this->Internals->TimeKeeper = smmodel->findItem<pqTimeKeeper*>(proxy);
    }
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
  if(this->session()->IsMultiClients())
    {
    vtkSMSessionClient* currentSession = vtkSMSessionClient::SafeDownCast(this->session());
    if(currentSession)
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
void pqServer::setResource(const pqServerResource &server_resource)
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
  if(this->Session && !this->Session->GetPendingProgress())
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << "HeartBeat"
           << vtkClientServerStream::End;
    this->Session->ExecuteStream(vtkPVSession::SERVERS, stream, true);
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
  return 1*60*1000; // 1 minutes.
}

//-----------------------------------------------------------------------------
void pqServer::setCoincidentTopologyResolutionMode(int mode)
{
  vtkSMPropertyHelper(this->GlobalMapperPropertiesProxy,
    "Mode").Set(mode);
  this->GlobalMapperPropertiesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqServer::setPolygonOffsetParameters(double factor, double units)
{
  vtkSMPropertyHelper helper(this->GlobalMapperPropertiesProxy,
    "PolygonOffsetParameters");
  helper.Set(0, factor);
  helper.Set(1, units);
  this->GlobalMapperPropertiesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqServer::setPolygonOffsetFaces(bool offset_faces)
{
  vtkSMPropertyHelper(this->GlobalMapperPropertiesProxy,
    "PolygonOffsetFaces").Set(offset_faces? 1 : 0);
  this->GlobalMapperPropertiesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqServer::setZShift(double shift)
{
  vtkSMPropertyHelper(this->GlobalMapperPropertiesProxy,
    "ZShift").Set(shift);
  this->GlobalMapperPropertiesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqServer::setGlobalImmediateModeRendering(bool val)
{
  vtkSMPropertyHelper(this->GlobalMapperPropertiesProxy,
    "GlobalImmediateModeRendering").Set(val? 1 : 0);
  this->GlobalMapperPropertiesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqServer::setCoincidentTopologyResolutionModeSetting(int mode)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->setValue("/server/GlobalMapperProperties/Mode", mode);

  // update all existing servers.
  pqServer::updateGlobalMapperProperties();
}

//-----------------------------------------------------------------------------
int pqServer::coincidentTopologyResolutionModeSetting()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  return settings->value("/server/GlobalMapperProperties/Mode",
    VTK_RESOLVE_SHIFT_ZBUFFER).toInt();
}

//-----------------------------------------------------------------------------
void pqServer::setPolygonOffsetParametersSetting(double factor, double units)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->setValue("/server/GlobalMapperProperties/PolygonOffsetFactor",
    factor);
  settings->setValue("/server/GlobalMapperProperties/PolygonOffsetUnits",
    units);

  // update all existing servers.
  pqServer::updateGlobalMapperProperties();
}

//-----------------------------------------------------------------------------
void pqServer::polygonOffsetParametersSetting(double &factor, double &units)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  factor = settings->value("/server/GlobalMapperProperties/PolygonOffsetFactor",
    1.0).toDouble();
  units = settings->value("/server/GlobalMapperProperties/PolygonOffsetUnits",
    1.0).toDouble();
}

//-----------------------------------------------------------------------------
void pqServer::setPolygonOffsetFacesSetting(bool value)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->setValue("/server/GlobalMapperProperties/OffsetFaces", value);

  // update all existing servers.
  pqServer::updateGlobalMapperProperties();
}

//-----------------------------------------------------------------------------
bool pqServer::polygonOffsetFacesSetting()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  return settings->value("/server/GlobalMapperProperties/OffsetFaces",
    true).toBool();
}

//-----------------------------------------------------------------------------
void pqServer::setZShiftSetting(double shift)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->setValue("/server/GlobalMapperProperties/ZShift", shift);

  // update all existing servers.
  pqServer::updateGlobalMapperProperties();

}

//-----------------------------------------------------------------------------
double pqServer::zShiftSetting()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  return settings->value("/server/GlobalMapperProperties/ZShift",
    2.0e-3).toDouble();
}

//-----------------------------------------------------------------------------
void pqServer::setGlobalImmediateModeRenderingSetting(bool val)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->setValue(
    "/server/GlobalMapperProperties/GlobalImmediateModeRendering", val);

  // update all existing servers.
  pqServer::updateGlobalMapperProperties();
}

//-----------------------------------------------------------------------------
bool pqServer::globalImmediateModeRenderingSetting()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  return settings->value(
    "/server/GlobalMapperProperties/GlobalImmediateModeRendering",
    false).toBool();
}

//-----------------------------------------------------------------------------
void pqServer::updateGlobalMapperProperties()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  foreach (pqServer* server, servers)
    {
    server->setCoincidentTopologyResolutionMode(
      pqServer::coincidentTopologyResolutionModeSetting());

    double factor, units;
    pqServer::polygonOffsetParametersSetting(factor, units);
    server->setPolygonOffsetParameters(factor, units);

    server->setPolygonOffsetFaces(pqServer::polygonOffsetFacesSetting());

    server->setZShift(pqServer::zShiftSetting());

    server->setGlobalImmediateModeRendering(
      pqServer::globalImmediateModeRenderingSetting());
    }
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
  if (sessionClient && sessionClient->IsNotBusy() && !this->isProgressPending())
    {
    // process all server-notification events.
    while (vtkProcessModule::GetProcessModule()->GetNetworkAccessManager()->ProcessEvents(1) == 1)
      {
      }
    foreach(pqView* view, pqApplicationCore::instance()->findChildren<pqView*>())
      {
      vtkSMViewProxy* viewProxy = view->getViewProxy();
      if(viewProxy && viewProxy->HasDirtyRepresentation())
        {
        view->render();
        }
      }
    }
  this->IdleCollaborationTimer.start();
}

//-----------------------------------------------------------------------------
void pqServer::onCollaborationCommunication(vtkObject* vtkNotUsed(src),
                                            unsigned long event_,
                                            void* vtkNotUsed(method),
                                            void* data)
{
  int userId;
  QString userName;
  switch(event_)
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
void pqServer::sendToOtherClients(vtkSMMessage* msg)
{
  if(this->Internals->CollaborationCommunicator)
    {
    this->Internals->CollaborationCommunicator->SendToOtherClients(msg);
    }
}
//-----------------------------------------------------------------------------
bool pqServer::isProcessingPending() const
{
  // check with the network access manager if there are any messages to receive
  // from the server.
  bool retVal = vtkProcessModule::GetProcessModule()->
    GetNetworkAccessManager()->GetNetworkEventsAvailable();
  return retVal;
}
