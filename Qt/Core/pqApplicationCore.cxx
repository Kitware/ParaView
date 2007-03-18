/*=========================================================================

   Program:   ParaView
   Module:    pqApplicationCore.cxx

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

========================================================================*/
#include "pqApplicationCore.h"

#include "QVTKWidget.h"
// ParaView Server Manager includes.
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMRenderModuleProxy.h"

#include <vtksys/SystemTools.hxx>

// Qt includes.
#include <QApplication>
#include <QDomDocument>
#include <QFile>
#include <QMap>
#include <QPointer>
#include <QSize>
#include <QtDebug>

// ParaView includes.
#include "pq3DWidgetFactory.h"
#include "pqCoreInit.h"
#include "pqDisplayPolicy.h"
#include "pqLinksModel.h"
#include "pqLookupTableManager.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProgressManager.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerResources.h"
#include "pqServerStartups.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqStandardViewModules.h"
#include "pqXMLUtil.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqApplicationCoreInternal
{
public:
  pqServerManagerObserver* ServerManagerObserver;
  pqServerManagerModel* ServerManagerModel;
  pqObjectBuilder* ObjectBuilder;
  pq3DWidgetFactory* WidgetFactory;
  pqServerManagerSelectionModel* SelectionModel;
  QPointer<pqDisplayPolicy> DisplayPolicy;
  vtkSmartPointer<vtkSMStateLoader> StateLoader;
  QPointer<pqLookupTableManager> LookupTableManager;
  pqLinksModel LinksModel;
  pqPluginManager* PluginManager;
  pqProgressManager* ProgressManager;

  QPointer<pqUndoStack> UndoStack;

  QMap<QString, QPointer<QObject> > RegisteredManagers;

  QString OrganizationName;
  QString ApplicationName;
  QPointer<pqServerResources> ServerResources;
  QPointer<pqServerStartups> ServerStartups;
  QPointer<pqSettings> Settings;
};


//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::Instance = 0;

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::instance()
{
  return pqApplicationCore::Instance;
}

//-----------------------------------------------------------------------------
pqApplicationCore::pqApplicationCore(QObject* p/*=null*/)
  : QObject(p)
{
  // initialize statics in case we're a static library
  pqCoreInit();

  this->Internal = new pqApplicationCoreInternal();

  this->Internal->ApplicationName = "ParaViewBasedApplication";
  this->Internal->OrganizationName = "Humanity";

  // *  Create pqServerManagerObserver first. This is the vtkSMProxyManager observer.
  this->Internal->ServerManagerObserver = new pqServerManagerObserver(this);

  // *  Create pqServerManagerModel.
  //    This is the representation builder for the ServerManager state.
  this->Internal->ServerManagerModel = new pqServerManagerModel(this);

  // *  Make signal-slot connections between ServerManagerObserver and ServerManagerModel.
  this->connect(this->Internal->ServerManagerObserver, this->Internal->ServerManagerModel);


  // *  Create the pqObjectBuilder. This is used to create pipeline objects.
  this->Internal->ObjectBuilder = new pqObjectBuilder(this);

  if (!pqApplicationCore::Instance)
    {
    pqApplicationCore::Instance = this;
    }
  
  this->Internal->PluginManager = new pqPluginManager(this);

  // * Create various factories.
  this->Internal->WidgetFactory = new pq3DWidgetFactory(this);

  // * Setup the selection model.
  this->Internal->SelectionModel = new pqServerManagerSelectionModel(
    this->Internal->ServerManagerModel, this);
  
  this->Internal->DisplayPolicy = new pqDisplayPolicy(this);

  this->Internal->ProgressManager = new pqProgressManager(this);

  // add standard views
  this->Internal->PluginManager->addInterface(
    new pqStandardViewModules(this->Internal->PluginManager));

  this->LoadingState = false;
}

//-----------------------------------------------------------------------------
pqApplicationCore::~pqApplicationCore()
{
  if (pqApplicationCore::Instance == this)
    {
    pqApplicationCore::Instance = 0;
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::connect(pqServerManagerObserver* pdata, 
  pqServerManagerModel* smModel)
{
  QObject::connect(pdata, SIGNAL(sourceRegistered(QString, vtkSMProxy*)),
    smModel, SLOT(onAddSource(QString, vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(sourceUnRegistered(QString, vtkSMProxy*)),
    smModel, SLOT(onRemoveSource(QString, vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(connectionCreated(vtkIdType)),
    smModel, SLOT(onAddServer(vtkIdType)));
  QObject::connect(pdata, SIGNAL(connectionClosed(vtkIdType)),
    smModel, SLOT(onRemoveServer(vtkIdType)));
  QObject::connect(pdata, SIGNAL(viewModuleRegistered(QString, 
        vtkSMAbstractViewModuleProxy*)),
    smModel, SLOT(onAddViewModule(QString, vtkSMAbstractViewModuleProxy*)));
  QObject::connect(pdata, SIGNAL(viewModuleUnRegistered(vtkSMAbstractViewModuleProxy*)),
    smModel, SLOT(onRemoveViewModule(vtkSMAbstractViewModuleProxy*)));
  QObject::connect(pdata, 
    SIGNAL(displayRegistered(QString, vtkSMProxy*)),
    smModel, SLOT(onAddDisplay(QString, vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(displayUnRegistered(vtkSMProxy*)),
    smModel, SLOT(onRemoveDisplay(vtkSMProxy*)));
  QObject::connect(
    pdata, SIGNAL(proxyRegistered(QString, QString, vtkSMProxy*)),
    smModel, SLOT(onProxyRegistered(QString, QString, vtkSMProxy*)));
  QObject::connect(
    pdata, SIGNAL(proxyUnRegistered(QString, QString, vtkSMProxy*)),
    smModel, SLOT(onProxyUnRegistered(QString, QString, vtkSMProxy*)));
      
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setLookupTableManager(pqLookupTableManager* mgr)
{
  this->Internal->LookupTableManager = mgr;
}

//-----------------------------------------------------------------------------
pqLookupTableManager* pqApplicationCore::getLookupTableManager() const
{
  return this->Internal->LookupTableManager;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setUndoStack(pqUndoStack* stack)
{
  this->Internal->UndoStack = stack;
}

//-----------------------------------------------------------------------------
pqUndoStack* pqApplicationCore::getUndoStack() const
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
pqObjectBuilder* pqApplicationCore::getObjectBuilder() const
{
  return this->Internal->ObjectBuilder;
}

//-----------------------------------------------------------------------------
pqServerManagerObserver* pqApplicationCore::getServerManagerObserver()
{
  return this->Internal->ServerManagerObserver;
}

//-----------------------------------------------------------------------------
pqServerManagerModel* pqApplicationCore::getServerManagerModel()
{
  return this->Internal->ServerManagerModel;
}

//-----------------------------------------------------------------------------
pq3DWidgetFactory* pqApplicationCore::get3DWidgetFactory()
{
  return this->Internal->WidgetFactory;
}

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel* pqApplicationCore::getSelectionModel()
{
  return this->Internal->SelectionModel;
}

//-----------------------------------------------------------------------------
pqLinksModel* pqApplicationCore::getLinksModel()
{
  return &this->Internal->LinksModel;
}

//-----------------------------------------------------------------------------
pqPluginManager* pqApplicationCore::getPluginManager()
{
  return this->Internal->PluginManager;
}

//-----------------------------------------------------------------------------
pqProgressManager* pqApplicationCore::getProgressManager() const
{
  return this->Internal->ProgressManager;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setDisplayPolicy(pqDisplayPolicy* policy) 
{
  this->Internal->DisplayPolicy = policy;
}

//-----------------------------------------------------------------------------
pqDisplayPolicy* pqApplicationCore::getDisplayPolicy() const
{
  return this->Internal->DisplayPolicy;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::registerManager(const QString& function, 
  QObject* manager)
{
  if (this->Internal->RegisteredManagers.contains(function) &&
    this->Internal->RegisteredManagers[function] != 0)
    {
    qDebug() << "Replacing existing manager for function : " 
      << function;
    }
  this->Internal->RegisteredManagers[function] = manager;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::unRegisterManager(const QString& function)
{
  this->Internal->RegisteredManagers.remove(function);
}

//-----------------------------------------------------------------------------
QObject* pqApplicationCore::manager(const QString& function)
{
  QMap<QString, QPointer<QObject> >::iterator iter =
    this->Internal->RegisteredManagers.find(function);
  if (iter != this->Internal->RegisteredManagers.end())
    {
    return iter.value();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::removeServer(pqServer* server)
{
  if (!server)
    {
    qDebug() << "No server to remove.";
    return;
    }

  this->getServerManagerModel()->beginRemoveServer(server);
  this->getObjectBuilder()->destroyAllProxies(server);
  pqServer::disconnect(server);
  this->getServerManagerModel()->endRemoveServer();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setStateLoader(vtkSMStateLoader* loader)
{
  this->Internal->StateLoader = loader;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::saveState(vtkPVXMLElement* rootElement)
{
  // * Save the Proxy Manager state.

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  // Eventually proxy manager will save state for each connection separately.
  // For now, we only have one connection, so simply save it.
  vtkPVXMLElement* smState = vtkPVXMLElement::New();
  smState->SetName("ServerManagerState");
  rootElement->AddNestedElement(smState);
  smState->Delete();

  pxm->SaveState(smState);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(vtkPVXMLElement* rootElement, 
                                  pqServer* server,
                                  vtkSMStateLoader* arg_loader/*=NULL*/)
{
  if (!server)
    {
    return ;
    }

  vtkSmartPointer<vtkSMStateLoader> loader = arg_loader;
  if (!loader)
    {
    loader = this->Internal->StateLoader;
    }
  if (!loader)
    {
    loader.TakeReference(vtkSMPQStateLoader::New());
    rootElement = pqXMLUtil::FindNestedElementByName(rootElement,
      "ServerManagerState");
    }

  vtkSMPQStateLoader* pqLoader = vtkSMPQStateLoader::SafeDownCast(loader);
  if (pqLoader)
    {
    // tell the state loader to use the existing render modules before creating new ones
    vtkSMRenderModuleProxy *smRen;
    for(unsigned int i=0; 
      i<server->GetRenderModule()->GetNumberOfRenderModules(); i++)
      {
      if( (smRen = dynamic_cast<vtkSMRenderModuleProxy*>(
            server->GetRenderModule()->GetRenderModule(i))) )
        {
        pqLoader->AddPreferredRenderModule(smRen);
        }
      }
    pqLoader->SetMultiViewRenderModuleProxy(server->GetRenderModule());
    }

  this->LoadingState = true;

  if (rootElement)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->LoadState(rootElement, server->GetConnectionID(), loader);
    pxm->UpdateRegisteredProxiesInOrder(0);
    }


  if (pqLoader)
    {
    // this is necessary to avoid unnecesary references to the render module,
    // enabling the proxy to be cleaned up before server disconnect.
    pqLoader->SetMultiViewRenderModuleProxy(0);
    // delete any unused rendermodules from state loader
    pqLoader->ClearPreferredRenderModules();
    }

  QApplication::processEvents();
  this->render();

  this->LoadingState = false;
  emit this->stateLoaded();
}

//-----------------------------------------------------------------------------
pqServerResources& pqApplicationCore::serverResources()
{
  if(!this->Internal->ServerResources)
    {
    this->Internal->ServerResources = new pqServerResources(this);
    this->Internal->ServerResources->load(*this->settings());
    }
    
  return *this->Internal->ServerResources;
}

//-----------------------------------------------------------------------------
pqServerStartups& pqApplicationCore::serverStartups()
{
  if(!this->Internal->ServerStartups)
    {
    this->Internal->ServerStartups = new pqServerStartups(this);
    
    // Load default settings ...
    QFile file(QApplication::applicationDirPath() + "/default_servers.pvsc");
    if(file.exists())
      {
      QDomDocument xml;
      QString error_message;
      int error_line = 0;
      int error_column = 0;
      if(xml.setContent(&file, false, &error_message, &error_line, &error_column))
        {
        this->Internal->ServerStartups->load(xml);
        }
      else
        {
        qWarning() << "Error loading default_servers.pvsc: " << error_message 
          << " line: " << error_line << " column: " << error_column;
        }
      }
    
    // Load user settings ...
    this->Internal->ServerStartups->load(*this->settings());
    }
    
  return *this->Internal->ServerStartups;
}

//-----------------------------------------------------------------------------
pqSettings* pqApplicationCore::settings()
{
  if ( !this->Internal->Settings )
    {
    if ( this->Internal->OrganizationName.isEmpty() ||
      this->Internal->ApplicationName.isEmpty() )
      {
      return 0;
      }
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    if (options && options->GetDisableRegistry())
      {
      this->Internal->Settings = new pqSettings(this->Internal->OrganizationName,
        this->Internal->ApplicationName + ".DisabledRegistry", this);
      this->Internal->Settings->clear();
      }
    else
      {
      this->Internal->Settings = new pqSettings(this->Internal->OrganizationName,
        this->Internal->ApplicationName, this);
      }
    }
  return this->Internal->Settings;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setApplicationName(const QString& an)
{
  this->Internal->ApplicationName = an;
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::applicationName()
{
  return this->Internal->ApplicationName;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setOrganizationName(const QString& on)
{
  this->Internal->OrganizationName = on;
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::organizationName()
{
  return this->Internal->OrganizationName;
}

//-----------------------------------------------------------------------------
pqServer* pqApplicationCore::createServer(const pqServerResource& resource)
{
  // Create a modified version of the resource that only contains server information
  const pqServerResource server_resource = resource.schemeHostsPorts();

  // See if the server is already created.
  pqServerManagerModel *smModel = this->getServerManagerModel();
  pqServer *server = smModel->getServer(server_resource);
  if(!server)
    {
    // TEMP: ParaView only allows one server connection. Remove this
    // code when it supports multiple server connections.
    if(smModel->getNumberOfServers() > 0)
      {
      this->removeServer(smModel->getServerByIndex(0));
      }

    // Based on the server resource, create the correct type of server ...
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    vtkIdType id = vtkProcessModuleConnectionManager::GetNullConnectionID();
    if(server_resource.scheme() == "builtin")
      {
      id = pm->ConnectToSelf();
      }
    else if(server_resource.scheme() == "cs")
      {
      id = pm->ConnectToRemote(
        resource.host().toAscii().data(),
        resource.port(11111));
      }
    else if(server_resource.scheme() == "csrc")
      {
      qWarning() << "Server reverse connections not supported yet\n";
      }
    else if(server_resource.scheme() == "cdsrs")
      {
      id = pm->ConnectToRemote(
        server_resource.dataServerHost().toAscii().data(),
        server_resource.dataServerPort(11111),
        server_resource.renderServerHost().toAscii().data(),
        server_resource.renderServerPort(22221));
      }
    else if(server_resource.scheme() == "cdsrsrc")
      {
      qWarning() << "Data server/render server reverse connections not supported yet\n";
      }
    else
      {
      qCritical() << "Unknown server type: " << server_resource.scheme() << "\n";
      }

    if(id != vtkProcessModuleConnectionManager::GetNullConnectionID())
      {
      if(server_resource.scheme() != "builtin")
        {
        // Synchronize options with the server.
        // TODO: This again will work more reliably once we have separate
        // PVOptions per connection.
        pm->SynchronizeServerClientOptions(id);
        }

      server = smModel->getServer(id);
      server->setResource(server_resource);
      emit this->finishedAddingServer(server);
      }
    }

  return server;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::render()
{
  QList<pqGenericViewModule*> list = 
    this->getServerManagerModel()->getViewModules(NULL);
  foreach(pqGenericViewModule* view, list)
    {
    view->render();
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::prepareProgress()
{
  if (this->Internal->ProgressManager)
    {
    this->Internal->ProgressManager->setEnableProgress(true);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::cleanupPendingProgress()
{
  if (this->Internal->ProgressManager)
    {
    this->Internal->ProgressManager->setEnableProgress(false);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::sendProgress(const char* name, int value)
{
  QString message = name;
  if (this->Internal->ProgressManager)
    {
    this->Internal->ProgressManager->setProgress(message, value);
    }
}


