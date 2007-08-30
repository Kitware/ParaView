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
#include "vtkSMRenderViewProxy.h"

// Qt includes.
#include <QCoreApplication>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"

class pqServer::pqInternals
{
public:
  QPointer<pqTimeKeeper> TimeKeeper;

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

  this->RenderViewXMLName = 
    vtkSMRenderViewProxy::GetSuggestedRenderViewType(
      this->ConnectionID);
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
vtkSMRenderViewProxy* pqServer::newRenderView()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  return vtkSMRenderViewProxy::SafeDownCast(
    pxm->NewProxy("views", this->RenderViewXMLName.toAscii().data()));
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
