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

#include "vtkClientServerStream.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkToolkits.h"

// Qt includes.
#include <QCoreApplication>
#include <QtDebug>
#include <QTimer>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimeKeeper.h"

class pqServer::pqInternals
{
public:
  QPointer<pqTimeKeeper> TimeKeeper;
  // Used to send an heart beat message to the server to avoid 
  // inactivity timeouts.
  QTimer HeartbeatTimer;

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
  this->ConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqServer::initialize()
{
  // Setup the Connection TimeKeeper.
  // Currently, we are keeping seperate times per connection. Once we start
  // supporting multiple connections, we may want to the link the
  // connection times together.
  this->createTimeKeeper();

  // Create the GlobalMapperPropertiesProxy.
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy("misc", "GlobalMapperProperties");
  proxy->SetConnectionID(this->ConnectionID);
  proxy->UpdateVTKObjects();
  pxm->RegisterProxy("temp_prototypes", "GlobalMapperProperties", proxy);
  this->GlobalMapperPropertiesProxy = proxy;
  proxy->Delete();

  this->updateGlobalMapperProperties();
}

//-----------------------------------------------------------------------------
pqTimeKeeper* pqServer::getTimeKeeper() const
{
  return this->Internals->TimeKeeper;
}

//-----------------------------------------------------------------------------
void pqServer::createTimeKeeper()
{
  // Set Global Time keeper.
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy("misc","TimeKeeper");
  proxy->SetConnectionID(this->ConnectionID);
  proxy->SetServers(vtkProcessModule::CLIENT);
  proxy->UpdateVTKObjects();
  pxm->RegisterProxy("timekeeper", "TimeKeeper", proxy);
  proxy->Delete();

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  this->Internals->TimeKeeper = smmodel->findItem<pqTimeKeeper*>(proxy);
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
QString pqServer::getRenderViewXMLName() const
{
  return "RenderView";
}

//-----------------------------------------------------------------------------
int pqServer::getNumberOfPartitions()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->GetNumberOfPartitions(this->ConnectionID);
}

//-----------------------------------------------------------------------------
bool pqServer::isRemote() const
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->IsRemote(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void pqServer::setResource(const pqServerResource &server_resource)
{
  this->Resource = server_resource;
  emit this->nameChanged(this);
}

//-----------------------------------------------------------------------------
void pqServer::getSupportedProxies(const QString& xmlgroup, QList<QString>& names)
{
  names.clear();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  unsigned int numProxies = pxm->GetNumberOfXMLProxies(
    xmlgroup.toAscii().data());
  for (unsigned int cc=0; cc <numProxies; cc++)
    {
    const char* name = pxm->GetXMLProxyName(xmlgroup.toAscii().data(),
      cc);
    if (name)
      {
      names.push_back(name);
      }
    }
}

//-----------------------------------------------------------------------------
vtkPVOptions* pqServer::getOptions() const
{
  return this->Options;
}

//-----------------------------------------------------------------------------
vtkPVServerInformation* pqServer::getServerInformation() const
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->GetServerInformation(this->GetConnectionID());
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
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID()
         << "GetProcessModule"
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::SERVERS, stream);
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
