/*=========================================================================

   Program:   ParaView
   Module:    pqApplicationCore.cxx

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

========================================================================*/
#include "pqApplicationCore.h"

// ParaView Server Manager includes.
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <vtksys/SystemTools.hxx>

// Qt includes.
#include <QApplication>
#include <QMap>
#include <QPointer>
#include <QSize>
#include <QtDebug>

// ParaView includes.
#include "pq3DWidgetFactory.h"
#include "pqAnimationScene.h"
#include "pqCoreInit.h"
#include "pqDisplayPolicy.h"
#include "pqEventDispatcher.h"
#include "pqLinksModel.h"
#include "pqLookupTableManager.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProgressManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerResources.h"
#include "pqServerStartups.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqStandardServerManagerModelInterface.h"
#include "pqStandardViewModules.h"
#include "pqUndoStack.h"
#include "pqXMLUtil.h"
#include "pqRenderView.h"

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

  this->setApplicationName("ParaViewBasedApplication");
  this->setOrganizationName("Humanity");

  // *  Create pqServerManagerObserver first. This is the vtkSMProxyManager observer.
  this->Internal->ServerManagerObserver = new pqServerManagerObserver(this);

  // *  Make signal-slot connections between ServerManagerObserver and ServerManagerModel.
  //this->connect(this->Internal->ServerManagerObserver, this->Internal->ServerManagerModel);

  this->Internal->ServerManagerModel = new pqServerManagerModel(
    this->Internal->ServerManagerObserver, this);

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

  // add standard server manager model interface
  this->Internal->PluginManager->addInterface(
    new pqStandardServerManagerModelInterface(this->Internal->PluginManager));

  this->LoadingState = false;
}

//-----------------------------------------------------------------------------
pqApplicationCore::~pqApplicationCore()
{
  // give chance to save before pqApplicationCore is gone
  delete this->Internal->ServerStartups;

  if (pqApplicationCore::Instance == this)
    {
    pqApplicationCore::Instance = 0;
    }
  delete this->Internal;

  // Unregister all proxies registered with the proxy manager.
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->UnRegisterProxies();
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
  QObject* _manager)
{
  if (this->Internal->RegisteredManagers.contains(function) &&
    this->Internal->RegisteredManagers[function] != 0)
    {
    qDebug() << "Replacing existing manager for function : " 
      << function;
    }
  this->Internal->RegisteredManagers[function] = _manager;
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
  vtkPVXMLElement* smState = pxm->SaveState();
  rootElement->AddNestedElement(smState);
  smState->Delete();

}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(vtkPVXMLElement* rootElement, 
  pqServer* server, vtkSMStateLoader* arg_loader/*=NULL*/,
  bool reuse_views/*=false*/)
{
  if (!server || !rootElement)
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
    // Create a default server manager state loader.
    // Since server manager state loader does not handle
    // any elements except "ServerManagerState",
    // we make that the root element.
    loader.TakeReference(vtkSMPQStateLoader::New());
    rootElement = pqXMLUtil::FindNestedElementByName(rootElement,
      "ServerManagerState");
    }

  vtkSMPQStateLoader* pqLoader = vtkSMPQStateLoader::SafeDownCast(loader);

  // Preferred view stuff needs to be deprecated BIG TIME! For now, I am letting
  // it be since the lookmark stuff uses it.
  QList<pqView*> current_views = 
    this->Internal->ServerManagerModel->findItems<pqView*>(server);
  foreach (pqView* view, current_views)
    {
    if (pqLoader && reuse_views && qobject_cast<pqRenderView*>(view))
      {
      pqLoader->AddPreferredView(view->getViewProxy());
      }
    else
      {
      this->Internal->ObjectBuilder->destroy(view);
      }
    }

  this->LoadingState = true;

  if (rootElement)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->LoadState(rootElement, server->GetConnectionID(), loader);
    }


  if (pqLoader)
    {
    // delete any unused rendermodules from state loader
    pqLoader->ClearPreferredViews();
    }

  pqEventDispatcher::processEventsAndWait(1);
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
    }
  return *this->Internal->ServerStartups;
}

//-----------------------------------------------------------------------------
pqSettings* pqApplicationCore::settings()
{
  if ( !this->Internal->Settings )
    {
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    if (options && options->GetDisableRegistry())
      {
      this->Internal->Settings = new pqSettings(QApplication::organizationName(),
        QApplication::applicationName() + ".DisabledRegistry", this);
      this->Internal->Settings->clear();
      }
    else
      {
      this->Internal->Settings = new pqSettings(QApplication::organizationName(),
        QApplication::applicationName(), this);
      }
    }
  return this->Internal->Settings;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setApplicationName(const QString& an)
{
  QApplication::setApplicationName(an);
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::applicationName()
{
  return QApplication::applicationName();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setOrganizationName(const QString& on)
{
  QApplication::setOrganizationName(on);
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::organizationName()
{
  return QApplication::organizationName();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::render()
{
  QList<pqView*> list = 
    this->Internal->ServerManagerModel->findItems<pqView*>();
  foreach(pqView* view, list)
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

//-----------------------------------------------------------------------------
void pqApplicationCore::quit()
{
  // As tempting as it is to connect this slot to 
  // aboutToQuit() signal, it doesn;t work since that signal is not
  // fired until the event loop exits, which doesn't happen until animation
  // stops playing.
  QList<pqAnimationScene*> scenes = 
    this->getServerManagerModel()->findItems<pqAnimationScene*>();
  foreach (pqAnimationScene* scene, scenes)
    {
    scene->pause();
    }
  QCoreApplication::instance()->quit();
}

