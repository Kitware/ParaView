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
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
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
#include <QPointer>
#include <QtDebug>
#include <QSize>

// ParaView includes.
#include "pq3DWidgetFactory.h"
#include "pqCoreInit.h"
#include "pqDisplayPolicy.h"
#include "pqLinksModel.h"
#include "pqLookupTableManager.h"
#include "pqOptions.h"
#include "pqPendingDisplayManager.h"
#include "pqPendingDisplayUndoElement.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProgressManager.h"
#include "pqReaderFactory.h"
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
#include "pqUndoStack.h"
#include "pqWriterFactory.h"
#include "pqXMLUtil.h"
#include "pqLookmarkManagerModel.h"

//-----------------------------------------------------------------------------
class pqApplicationCoreInternal
{
public:
  pqServerManagerObserver* ServerManagerObserver;
  pqServerManagerModel* ServerManagerModel;
  pqLookmarkManagerModel* LookmarkManagerModel;
  pqUndoStack* UndoStack;
  pqPipelineBuilder* PipelineBuilder;
  pq3DWidgetFactory* WidgetFactory;
  pqReaderFactory* ReaderFactory;
  pqWriterFactory* WriterFactory;
  pqServerManagerSelectionModel* SelectionModel;
  pqPendingDisplayManager* PendingDisplayManager;
  QPointer<pqDisplayPolicy> DisplayPolicy;
  vtkSmartPointer<vtkSMStateLoader> StateLoader;
  QPointer<pqLookupTableManager> LookupTableManager;
  pqLinksModel LinksModel;
  pqPluginManager PluginManager;
  pqProgressManager* ProgressManager;

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

  // *  Create the Undo/Redo stack.
  this->Internal->UndoStack = new pqUndoStack(false, this);

  // *  Create the pqPipelineBuilder. This is used to create pipeline objects.
  this->Internal->PipelineBuilder = new pqPipelineBuilder(this);
  this->Internal->PipelineBuilder->setUndoStack(this->Internal->UndoStack);

  if (!pqApplicationCore::Instance)
    {
    pqApplicationCore::Instance = this;
    }

  // * Create various factories.
  this->Internal->WidgetFactory = new pq3DWidgetFactory(this);
  this->Internal->ReaderFactory = new pqReaderFactory(this);
  this->Internal->WriterFactory = new pqWriterFactory(this);

  // * Setup the selection model.
  this->Internal->SelectionModel = new pqServerManagerSelectionModel(
    this->Internal->ServerManagerModel, this);
  
  this->Internal->PendingDisplayManager = new pqPendingDisplayManager(this);

  this->Internal->DisplayPolicy = new pqDisplayPolicy(this);

  this->Internal->ProgressManager = new pqProgressManager(this);

  // add standard views
  this->Internal->PluginManager.addInterface(
    new pqStandardViewModules(&this->Internal->PluginManager));

  // *  Create pqLookmarkManagerModel.
  //    This is the model for the application's collection of lookmarks.
  this->Internal->LookmarkManagerModel = new pqLookmarkManagerModel(this);

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
  pqServerManagerModel* smModel = this->getServerManagerModel();
  if (this->Internal->LookupTableManager)
    {
    QObject::disconnect(smModel, 0, this->Internal->LookupTableManager, 0);
    }
  this->Internal->LookupTableManager = mgr;
  if (mgr)
    {
    QObject::connect(smModel, SIGNAL(proxyAdded(pqProxy*)),
      mgr, SLOT(onAddProxy(pqProxy*)));
    }
}

//-----------------------------------------------------------------------------
pqLookupTableManager* pqApplicationCore::getLookupTableManager() const
{
  return this->Internal->LookupTableManager;
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
pqLookmarkManagerModel* pqApplicationCore::getLookmarkManagerModel()
{
  return this->Internal->LookmarkManagerModel;
}

//-----------------------------------------------------------------------------
pqUndoStack* pqApplicationCore::getUndoStack()
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
pqPipelineBuilder* pqApplicationCore::getPipelineBuilder()
{
  return this->Internal->PipelineBuilder;
}

//-----------------------------------------------------------------------------
pq3DWidgetFactory* pqApplicationCore::get3DWidgetFactory()
{
  return this->Internal->WidgetFactory;
}

//-----------------------------------------------------------------------------
pqReaderFactory* pqApplicationCore::getReaderFactory()
{
  return this->Internal->ReaderFactory;
}

//-----------------------------------------------------------------------------
pqWriterFactory* pqApplicationCore::getWriterFactory()
{
  return this->Internal->WriterFactory;
}

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel* pqApplicationCore::getSelectionModel()
{
  return this->Internal->SelectionModel;
}

//-----------------------------------------------------------------------------
pqPendingDisplayManager* pqApplicationCore::getPendingDisplayManager()
{
  return this->Internal->PendingDisplayManager;
}

//-----------------------------------------------------------------------------
pqLinksModel* pqApplicationCore::getLinksModel()
{
  return &this->Internal->LinksModel;
}

//-----------------------------------------------------------------------------
pqPluginManager* pqApplicationCore::getPluginManager()
{
  return &this->Internal->PluginManager;
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
void pqApplicationCore::removeSource(pqPipelineSource* source)
{
  if (!source)
    {
    qDebug() << "No source to remove.";
    return;
    }
  if (source->getNumberOfConsumers())
    {
    qDebug() << "Active source has consumers, cannot delete";
    return;
    }

  QList<pqGenericViewModule*> viewModules = source->getViewModules();

  // Make all inputs visible in views that the removed source
  // is currently visible.
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (filter)
    {
    QList<pqPipelineSource*> inputs = filter->getInputs();
    foreach(pqGenericViewModule* view, viewModules)
      {
      pqConsumerDisplay* src_disp = source->getDisplay(view);
      if (!src_disp || !src_disp->isVisible())
        {
        continue;
        }
      // For each input, if it is not visibile in any of the views
      // that the delete filter is visible, we make the input visible.
      for(int cc=0; cc < inputs.size(); ++cc)
        {
        pqPipelineSource* input = inputs[cc];
        pqConsumerDisplay* input_disp = input->getDisplay(view);
        if (input_disp && !input_disp->isVisible())
          {
          input_disp->setVisible(true);
          }
        }
      }
    }

  emit this->aboutToRemoveSource(source);
 
  this->getPipelineBuilder()->remove(source);


  foreach (pqGenericViewModule* view, viewModules)
    {
    view->render();
    }
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
  this->getPipelineBuilder()->deleteProxies(server);
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
    for(unsigned int i=0; i<server->GetRenderModule()->GetNumberOfProxies(); i++)
      {
      if( (smRen = dynamic_cast<vtkSMRenderModuleProxy*>(server->GetRenderModule()->GetProxy(i))) )
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

  // Clear undo stack.
  this->Internal->UndoStack->clear();

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

pqServerResources& pqApplicationCore::serverResources()
{
  if(!this->Internal->ServerResources)
    {
    this->Internal->ServerResources = new pqServerResources(this);
    this->Internal->ServerResources->load(*this->settings());
    }
    
  return *this->Internal->ServerResources;
}

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

static void SetDefaultInputArray(vtkSMProxy* Proxy, const char* PropertyName)
{
  if(vtkSMStringVectorProperty* const array =
    vtkSMStringVectorProperty::SafeDownCast(
      Proxy->GetProperty(PropertyName)))
    {
    Proxy->UpdateVTKObjects();
  
    QList<QVariant> domain = 
      pqSMAdaptor::getEnumerationPropertyDomain(array);

    for(int i = 0; i != domain.size(); ++i)
      {
      const QString name = domain[i].toString();
      array->SetElement(4, name.toAscii().data());
      array->UpdateDependentDomains();
      break;
      }
    }
}

pqPipelineSource* pqApplicationCore::createFilterForSource(const QString& xmlname,
                                        pqPipelineSource* input)
{
  if (!input)
    {
    qDebug() << "No source/filter active. Cannot createFilterForSource.";
    return NULL;
    }

  this->getUndoStack()->beginUndoSet(QString("Create ") + xmlname);

  pqPipelineSource* filter = this->getPipelineBuilder()->createSource(
    "filters", xmlname.toAscii().data(), input->getServer());

  if(filter)
    {
    this->getPipelineBuilder()->addConnection(input, filter);
    if (vtkSMProperty* const input_prop = filter->getProxy()->GetProperty("Input"))
      {
      input_prop->UpdateDependentDomains();
      }
    filter->setDefaultValues();

    // As a special-case, set a default implicit function for new Cut filters
    if(xmlname == "Cut")
      {
      if(vtkSMDoubleVectorProperty* const contours =
        vtkSMDoubleVectorProperty::SafeDownCast(
          filter->getProxy()->GetProperty("ContourValues")))
        {
        contours->SetNumberOfElements(1);
        contours->SetElement(0, 0.0);
        }
      }

    // As a special-case, set the default contour for new Contour filters
    if(xmlname == "Contour")
      {
      double min_value = 0.0;
      double max_value = 0.0;

      ::SetDefaultInputArray(filter->getProxy(), "SelectInputScalars");

      if(vtkSMDoubleVectorProperty* const contours =
        vtkSMDoubleVectorProperty::SafeDownCast(
          filter->getProxy()->GetProperty("ContourValues")))
        {
        if(vtkSMDoubleRangeDomain* const domain =
          vtkSMDoubleRangeDomain::SafeDownCast(
            contours->GetDomain("scalar_range")))
          {
          int min_exists = 0;
          min_value = domain->GetMinimum(0, min_exists);
          
          int max_exists = 0;
          max_value = domain->GetMaximum(0, max_exists);
          }

        contours->SetNumberOfElements(1);
        contours->SetElement(0, (min_value + max_value) * 0.5);
        }
      }

    // As a special-case, set a default point source for new StreamTracer filters
    if(xmlname == "StreamTracer")
      {
      ::SetDefaultInputArray(filter->getProxy(), "SelectInputVectors");
      }
    }

  emit this->finishSourceCreation(filter);
  this->getUndoStack()->endUndoSet();
  emit this->finishedAddingSource(filter);
  return filter;
}

pqPipelineSource* pqApplicationCore::createSourceOnServer(const QString& xmlname,
  pqServer* server)
{
  if (!server)
    {
    qDebug() << "No server specified. "
      << "Cannot createSourceOnServer.";
    return 0;
    }

  this->getUndoStack()->beginUndoSet(QString("Create ") + xmlname);

  pqPipelineSource* source = this->getPipelineBuilder()->createSource(
    "sources", xmlname.toAscii().data(), server);
  source->setDefaultValues();

  emit this->finishSourceCreation(source);
  this->getUndoStack()->endUndoSet();
  
  emit this->finishedAddingSource(source);
  return source;
}

pqPipelineSource* pqApplicationCore::createCompoundFilter(
                         const QString& name,
                         pqServer* server,
                         pqPipelineSource* input)
{
  this->getUndoStack()->beginUndoSet(QString("Create ") + name);

  pqPipelineSource* source = this->getPipelineBuilder()->createSource(
    NULL, name.toAscii().data(), server);
  
  vtkSMProperty* inputProperty = NULL;

  if(source)
    {
    inputProperty = source->getProxy()->GetProperty("Input");
    }

  if(inputProperty && input == NULL)
    {
    this->removeSource(source);
    source = NULL;
    qWarning() << "Cannot create custom filter without active input source.";
    }
  else if(inputProperty && input)
    {
    this->getPipelineBuilder()->addConnection(input, source);
    inputProperty->UpdateDependentDomains();
    }

  if(source)
    {
    source->setDefaultValues();
    }

  emit this->finishSourceCreation(source);
  this->getUndoStack()->endUndoSet();
  emit this->finishedAddingSource(source);
  return source;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::createReaderOnServer(
               const QString& filename,
               pqServer* server,
               QString whichReader)
{
  if (!server)
    {
    qDebug() << "No active server. Cannot create reader.";
    return 0;
    }
    
  if(this->getReaderFactory()->checkIfFileIsReadable(filename, server))
    {
    qDebug() << "File \"" << filename << "\"  cannot be read.";
    return NULL; 
    }
  
  this->getUndoStack()->beginUndoSet(QString("Create reader for ") + filename);

  if(whichReader == QString::null)
    {
    whichReader = this->getReaderFactory()->getReaderType(filename, server);
    }

  pqPipelineSource* reader= this->getReaderFactory()->createReader(whichReader,
                                                                   server);
  if (!reader)
    {
    this->getUndoStack()->endUndoSet();
    return NULL;
    }

  vtkSMProxy* proxy = reader->getProxy();

  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(proxy->NewPropertyIterator());
  piter->Begin();
  while(!piter->IsAtEnd())
    {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop->IsA("vtkSMStringVectorProperty"))
      {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      diter->Begin();
      while(!diter->IsAtEnd())
        {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
          {
          break;
          }
        diter->Next();
        }
      if (!diter->IsAtEnd())
        {
        break;
        }
      }
    piter->Next();
    }
  if (!piter->IsAtEnd())
    {
    pqSMAdaptor::setElementProperty(piter->GetProperty(), 
                                    filename);
    proxy->UpdateVTKObjects();
    }

  // Update pipeline information so that the reader
  // readers file headers and obtains necessary information.
  vtkSMSourceProxy::SafeDownCast(proxy)->UpdatePipelineInformation();
  proxy->UpdatePropertyInformation();
  reader->setDefaultValues();

  // Set the proxy name to be the same as the filename.
  reader->rename(
    vtksys::SystemTools::GetFilenameName(filename.toAscii().data()).c_str());

  emit this->finishSourceCreation(reader);
  this->getUndoStack()->endUndoSet();
  emit this->finishedAddingSource(reader);
  return reader;
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


