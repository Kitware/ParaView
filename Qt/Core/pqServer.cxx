/*=========================================================================

   Program: ParaView
   Module:    pqServer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

// VTK includes
#include <vtkToolkits.h>
#include <vtkObjectFactory.h>

// ParaView Server Manager includes
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

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

pqServer* pqServer::Create(const pqServerResource& resource)
{
  // Create a modified version of the resource that only contains server information
  const pqServerResource server_resource = resource.server();

  // Based on the server resource, create the correct type of server ...
  if(server_resource.scheme() == "builtin")
    {
    vtkProcessModule* const pm = vtkProcessModule::GetProcessModule();
    const vtkIdType id = pm->ConnectToSelf();
    if(id == vtkProcessModuleConnectionManager::GetNullConnectionID())
      {
      return 0;
      }
    pqServerManagerModel* const model = pqServerManagerModel::instance();
    pqServer* const server = model->getServer(id); //new pqServer(id, pm->GetOptions());
    server->Resource = server_resource;
    
    return server;
    }
  else if(server_resource.scheme() == "cs")
    {
    vtkProcessModule* const pm = vtkProcessModule::GetProcessModule();
    const vtkIdType id = pm->ConnectToRemote(resource.host().toAscii().data(), resource.port(11111));
    if(id == vtkProcessModuleConnectionManager::GetNullConnectionID())
      {
      return 0;
      }
    // Synchronize options with the server.
    // TODO: This again will work more reliably once we have separate PVOptions 
    // per connection.
    pm->SynchronizeServerClientOptions(id);

    pqServerManagerModel* const model = pqServerManagerModel::instance();
    pqServer* const server = model->getServer(id); //new pqServer(id, pm->GetOptions());
    server->Resource = server_resource;

    return server;
    }
  else if(server_resource.scheme() == "csrc")
    {
    qWarning() << "Server reverse connections not supported yet\n";
    return 0;
    }
  else if(server_resource.scheme() == "cdsrs")
    {
    vtkProcessModule* const pm = vtkProcessModule::GetProcessModule();
    const vtkIdType id = pm->ConnectToRemote(
      server_resource.dataServerHost().toAscii().data(),
      server_resource.dataServerPort(11111),
      server_resource.renderServerHost().toAscii().data(),
      server_resource.renderServerPort(21111));
    if(id == vtkProcessModuleConnectionManager::GetNullConnectionID())
      {
      return 0;
      }
    // Synchronize options with the server.
    // TODO: This again will work more reliably once we have separate PVOptions 
    // per connection.
    pm->SynchronizeServerClientOptions(id);
    
    pqServerManagerModel* const model = pqServerManagerModel::instance();
    pqServer* const server = model->getServer(id); //new pqServer(id, pm->GetOptions());
    server->Resource = server_resource;
    
    return server;
    }
  else if(server_resource.scheme() == "cdsrsrc")
    {
    qWarning() << "Data server/render server reverse connections not supported yet\n";
    return 0;
    }
  else
    {
    qCritical() << "Unknown server type: " << server_resource.scheme() << "\n";
    }

  return 0;
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
  pqServerManagerModelItem(_parent)
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

const pqServerResource& pqServer::getResource()
{
  return this->Resource;
}

vtkIdType pqServer::GetConnectionID()
{
  return this->ConnectionID;
}

//-----------------------------------------------------------------------------
int pqServer::getNumberOfPartitions()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->GetNumberOfPartitions(this->ConnectionID);

}
