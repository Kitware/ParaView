/*=========================================================================

   Program:   ParaQ
   Module:    pqServer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include <vtkToolkits.h>
#include <vtkObjectFactory.h>
#include <vtkProcessModuleGUIHelper.h>
#include <vtkPVOptions.h>
#include <vtkProcessModule.h>
#include <vtkProcessModuleConnectionManager.h>
#include <vtkPVServerInformation.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMMultiViewRenderModuleProxy.h>

// Qt includes.
#include <QCoreApplication>
#include <QtDebug>

// ParaQ includes.
#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServerManagerModel.h"
#include "pqPipelineData.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

pqServer* pqServer::CreateStandalone()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType id= pm->ConnectToSelf();
  if (id == vtkProcessModuleConnectionManager::GetNullConnectionID())
    {
    return NULL;
    }
  pqServerManagerModel* model = pqServerManagerModel::instance();
  pqServer* server = model->getServer(id); //new pqServer(id, pm->GetOptions());
  server->setFriendlyName("Builtin");
  return server;
  /*
  // Connection merely to the Self.
  // Process Module always instantiates a self connection.
  pqServer* server = 
    new pqServer(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::GetProcessModule()->GetOptions());
  server->setFriendlyName("BuiltIn");

  pqServerManagerModel* model = pqServerManagerModel::instance();
  // Since currently self connections are not new connections,
  // we, pretend that a new conneciton is created to that the
  // rest of the connection cogniscance still works. 
  model->onAddServer(server);
  return server;
  */
}

//-----------------------------------------------------------------------------
pqServer* pqServer::CreateConnection(const char* const hostName, int portNumber)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType id= pm->ConnectToRemote(hostName, portNumber);
  if (id == vtkProcessModuleConnectionManager::GetNullConnectionID())
    {
    return NULL;
    }
  // Synchronize options with the server.
  // TODO: This again will work more reliably once we have separate PVOptions 
  // per connection.
  pm->SynchronizeServerClientOptions(id);
  QString name;
  name.setNum(portNumber);
  name.prepend(":");
  name.prepend(hostName); 

  pqServerManagerModel* model = pqServerManagerModel::instance();
  pqServer* server = model->getServer(id); //new pqServer(id, pm->GetOptions());
  server->setFriendlyName(name);
  return server;
}

//-----------------------------------------------------------------------------
pqServer* pqServer::CreateConnection(const char* const ds_hostName, int ds_portNumber,
  const char* const rs_hostName, int rs_portNumber)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType id = !pm->ConnectToRemote(ds_hostName, ds_portNumber, rs_hostName,
      rs_portNumber);
  if (id == vtkProcessModuleConnectionManager::GetNullConnectionID())
    {
    return NULL;
    }
  // Synchronize options with the server.
  // TODO: This again will work more reliably once we have separate PVOptions 
  // per connection.
  pm->SynchronizeServerClientOptions(id);
  
  QString name1;
  name1.setNum(ds_portNumber);
  name1.prepend(":");
  name1.prepend(ds_hostName);

  QString name2;
  name2.setNum(rs_portNumber);
  name2.prepend(":");
  name2.prepend(rs_hostName);
  
  QString name = name1 + "--" + name2;

  //pqServer* server = new pqServer(id, pm->GetOptions());
  //server->setFriendlyName(name);
  //return server;

  pqServerManagerModel* model = pqServerManagerModel::instance();
  pqServer* server = model->getServer(id); //new pqServer(id, pm->GetOptions());
  server->setFriendlyName(name);
  return server;
}

//-----------------------------------------------------------------------------
void pqServer::disconnect(pqServer* server)
{
  // disconnect on the process module. The event handling will
  // clean up the connection.
  
  // For now, ensure that the render module is deleted before the connection is broken.
  // Eventually, the vtkSMProxyManager will support a close connection method
  // which will do proper connection proxy cleanup.
  server->RenderModule = NULL;
  vtkProcessModule::GetProcessModule()->Disconnect(
    server->GetConnectionID());
}

//-----------------------------------------------------------------------------
pqServer::pqServer(vtkIdType connectionID, vtkPVOptions* options, QObject* _parent) :
  pqPipelineModelItem(_parent), FriendlyName()
{
  this->ConnectionID = connectionID;
  this->Options = options;
  this->CreateRenderModule();
  this->setType(pqPipelineModel::Server);
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
}

//-----------------------------------------------------------------------------
void pqServer::CreateRenderModule()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMProxyManager* proxy_manager = vtkSMObject::GetProxyManager();

  vtkSMMultiViewRenderModuleProxy* render_module = 
    vtkSMMultiViewRenderModuleProxy::SafeDownCast(
      proxy_manager->NewProxy("rendermodules", "MultiViewRenderModule"));
  render_module->SetConnectionID(this->ConnectionID);

  const char* renderModuleName = 0;
  if (!pm->IsRemote(this->ConnectionID))
    {
    renderModuleName = "LODRenderModule";
    }
  if (!renderModuleName)
    {
    renderModuleName = this->Options->GetRenderModuleName();
    }
  if (!renderModuleName)
    {
    vtkPVServerInformation* server_info = pm->GetServerInformation(
      this->ConnectionID);
    if (server_info && server_info->GetUseIceT())
      {
      if (this->Options->GetTileDimensions()[0] )
        {
        renderModuleName = "IceTRenderModule";
        }
      else if(this->Options->GetClientMode())
        {
        renderModuleName = "IceTDesktopRenderModule";
        }
      } 
    else if(server_info) // && !server_info->GetUseIceT().
      {
      // \todo These render modules don't support multi view,
      // so, what to do if the server says it does not support IceT?
      if (this->Options->GetTileDimensions()[0] )
        {
        renderModuleName = "MultiDisplayRenderModule";
        }
      else if(this->Options->GetClientMode())
        {
        renderModuleName = "MPIRenderModule";
        }
      }
    }
  if (!renderModuleName)
    {
    // Last resort.
    renderModuleName = "LODRenderModule";
    }

  // this->Options->SetRenderModuleName(renderModuleName);
  render_module->SetRenderModuleName(renderModuleName);
  render_module->UpdateVTKObjects();
  this->RenderModule = render_module;
  render_module->Delete();
}

//-----------------------------------------------------------------------------
QString pqServer::getAddress() const
{
  QString address;
  if(this->Options)
    {
    address.setNum(this->Options->GetServerPort());
    address.prepend(":");
    address.prepend(this->Options->GetServerHostName());
    }

  return address;
}

//-----------------------------------------------------------------------------
void pqServer::setFriendlyName(const QString& name)
{
  this->FriendlyName = name;
  emit this->dataModified();
}

//-----------------------------------------------------------------------------
vtkSMMultiViewRenderModuleProxy* pqServer::GetRenderModule()
{
  return this->RenderModule.GetPointer();
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy* pqServer::newRenderModule()
{
  if (!this->RenderModule)
    {
    qDebug() << "No MultiDisplayRenderModule to create a new render module.";
    return NULL;
    }
  return vtkSMRenderModuleProxy::SafeDownCast(
    this->RenderModule->NewRenderModule());
}

