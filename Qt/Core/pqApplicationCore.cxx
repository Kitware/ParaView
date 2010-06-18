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

#include <vtksys/SystemTools.hxx>

// Qt includes.
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSize>
#include <QtDebug>

// ParaView includes.
#include "pq3DWidgetFactory.h"
#include "pqAnimationScene.h"
#include "pqCoreInit.h"
#include "pqCoreTestUtility.h"
#include "pqDisplayPolicy.h"
#include "pqEventDispatcher.h"
#include "pqLinksModel.h"
#include "pqLookupTableManager.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqOutputWindowAdapter.h"
#include "pqOutputWindow.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProgressManager.h"
#include "pqRenderView.h"
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
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMApplication.h"
#include "vtkSmartPointer.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMWriterFactory.h"

//-----------------------------------------------------------------------------
class pqApplicationCore::pqInternals
{
public:
  vtkSmartPointer<vtkSMGlobalPropertiesManager> GlobalPropertiesManager;
  QMap<QString, QPointer<QObject> > RegisteredManagers;
};

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::Instance = 0;

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::instance()
{
  return pqApplicationCore::Instance;
}

//-----------------------------------------------------------------------------
pqApplicationCore::pqApplicationCore(int& argc, char** argv, pqOptions* options,
  QObject* parentObject)
  : QObject(parentObject)
{
  vtkSmartPointer<pqOptions> defaultOptions;
  if (!options)
    {
    defaultOptions = vtkSmartPointer<pqOptions>::New();
    options = defaultOptions;
    }
  this->Options = options;

  // Create output window before initializing server manager.
  this->createOutputWindow();
  vtkInitializationHelper::Initialize(argc, argv, options);
  this->constructor();
  this->FinalizeOnExit = true;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::constructor()
{
  // Only 1 pqApplicationCore instance can be created.
  Q_ASSERT(pqApplicationCore::Instance == NULL);
  pqApplicationCore::Instance = this;

  this->LookupTableManager = NULL;
  this->UndoStack = NULL;
  this->ServerResources = NULL;
  this->ServerStartups = NULL;
  this->Settings = NULL;

  // initialize statics in case we're a static library
  pqCoreInit();

  this->Internal = new pqInternals();

  // *  Create pqServerManagerObserver first. This is the vtkSMProxyManager observer.
  this->ServerManagerObserver = new pqServerManagerObserver(this);

  // *  Make signal-slot connections between ServerManagerObserver and ServerManagerModel.
  this->ServerManagerModel = new pqServerManagerModel(
    this->ServerManagerObserver, this);

  // *  Create the pqObjectBuilder. This is used to create pipeline objects.
  this->ObjectBuilder = new pqObjectBuilder(this);

  this->PluginManager = new pqPluginManager(this);

  // * Create various factories.
  this->WidgetFactory = new pq3DWidgetFactory(this);

  // * Setup the selection model.
  this->SelectionModel = new pqServerManagerSelectionModel(
    this->ServerManagerModel, this);
  
  this->DisplayPolicy = new pqDisplayPolicy(this);

  this->ProgressManager = new pqProgressManager(this);

  // add standard server manager model interface
  this->PluginManager->addInterface(
    new pqStandardServerManagerModelInterface(this->PluginManager));

  this->LinksModel = new pqLinksModel(this);

  this->LoadingState = false;
  QObject::connect(this->ServerManagerObserver,
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(onStateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(this->ServerManagerObserver,
    SIGNAL(stateSaved(vtkPVXMLElement*)),
    this, SLOT(onStateSaved(vtkPVXMLElement*)));
}

//-----------------------------------------------------------------------------
pqApplicationCore::~pqApplicationCore()
{
  // Ensure that startup plugins get a chance to cleanup before pqApplicationCore is gone.
  delete this->PluginManager;
  this->PluginManager = 0;

  // give chance to save before pqApplicationCore is gone
  delete this->ServerStartups;
  this->ServerStartups = 0;

  // Ensure that all managers are deleted.
  delete this->WidgetFactory;
  this->WidgetFactory = 0;

  delete this->LinksModel;
  this->LinksModel = 0;

  delete this->ObjectBuilder;
  this->ObjectBuilder = 0;

  delete this->ProgressManager;
  this->ProgressManager = 0;

  delete this->ServerManagerModel;
  this->ServerManagerModel = 0;

  delete this->ServerManagerObserver;
  this->ServerManagerObserver = 0;

  delete this->SelectionModel;
  this->SelectionModel = 0;


  delete this->ServerResources;
  this->ServerResources = 0;

  delete this->Settings;
  this->Settings = 0;

  
  // We don't call delete on these since we have already setup parent on these
  // correctly so they will be deleted. It's possible that the user calls delete
  // on these explicitly in which case we end up with segfaults.
  this->LookupTableManager = 0;
  this->DisplayPolicy = 0;
  this->UndoStack = 0;

  // Delete all children, which clears up all managers etc. before the server
  // manager application is finalized.
  delete this->Internal;

  delete this->TestUtility;

  if (pqApplicationCore::Instance == this)
    {
    pqApplicationCore::Instance = 0;
    }

  if (this->FinalizeOnExit)
    {
    vtkInitializationHelper::Finalize();
    }
  vtkOutputWindow::SetInstance(NULL);
  delete this->OutputWindow;
  this->OutputWindow = NULL;
  this->OutputWindowAdapter->Delete();
  this->OutputWindowAdapter= 0;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::createOutputWindow()
{
  // Set up error window.
  pqOutputWindowAdapter* owAdapter = pqOutputWindowAdapter::New();
  this->OutputWindow = new pqOutputWindow(0);
  this->OutputWindow->setAttribute(Qt::WA_QuitOnClose, false);
  this->OutputWindow->connect(owAdapter,
    SIGNAL(displayText(const QString&)), SLOT(onDisplayText(const QString&)));
  this->OutputWindow->connect(owAdapter,
    SIGNAL(displayErrorText(const QString&)), SLOT(onDisplayErrorText(const QString&)));
  this->OutputWindow->connect(owAdapter,
    SIGNAL(displayWarningText(const QString&)), SLOT(onDisplayWarningText(const QString&)));
  this->OutputWindow->connect(owAdapter,
    SIGNAL(displayGenericWarningText(const QString&)),
    SLOT(onDisplayGenericWarningText(const QString&)));
  vtkOutputWindow::SetInstance(owAdapter);
  this->OutputWindowAdapter = owAdapter;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setLookupTableManager(pqLookupTableManager* mgr)
{
  this->LookupTableManager = mgr;
  if (mgr)
    {
    mgr->setParent(this);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setUndoStack(pqUndoStack* stack)
{
  if (stack != this->UndoStack)
    {
    this->UndoStack = stack;
    if (stack)
      {
      stack->setParent(this);
      }
    emit this->undoStackChanged(stack);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setDisplayPolicy(pqDisplayPolicy* policy) 
{
  delete this->DisplayPolicy;
  this->DisplayPolicy = policy;
  if (policy)
    {
    policy->setParent(this);
    }
}

//-----------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* pqApplicationCore::getGlobalPropertiesManager()
{
  if (!this->Internal->GlobalPropertiesManager)
    {
    // Setup the application's "GlobalProperties" proxy.
    // This is used to keep track of foreground color etc.
    this->Internal->GlobalPropertiesManager =
      vtkSmartPointer<vtkSMGlobalPropertiesManager>::New();
    this->Internal->GlobalPropertiesManager->InitializeProperties("misc",
      "GlobalProperties");
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->SetGlobalPropertiesManager("ParaViewProperties",
      this->Internal->GlobalPropertiesManager);

    // load settings.
    this->loadGlobalPropertiesFromSettings();
    }
  return this->Internal->GlobalPropertiesManager;
}

#define SET_COLOR_MACRO(settingkey, defaultvalue, propertyname)\
  color = _settings->value(settingkey, defaultvalue).value<QColor>();\
  rgb[0] = color.redF();\
  rgb[1] = color.greenF();\
  rgb[2] = color.blueF();\
  vtkSMPropertyHelper(mgr, propertyname).Set(rgb, 3);

//-----------------------------------------------------------------------------
void pqApplicationCore::loadGlobalPropertiesFromSettings()
{
  vtkSMGlobalPropertiesManager* mgr = this->getGlobalPropertiesManager();
  QColor color;
  double rgb[3];
  pqSettings* _settings = this->settings();
  SET_COLOR_MACRO(
    "GlobalProperties/ForegroundColor",
    QColor::fromRgbF(1, 1, 1),
    "ForegroundColor");
  SET_COLOR_MACRO(
    "GlobalProperties/SurfaceColor",
    QColor::fromRgbF(1, 1, 1),
    "SurfaceColor");
  SET_COLOR_MACRO(
    "GlobalProperties/BackgroundColor",
    QColor::fromRgbF(0.32, 0.34, 0.43),
    "BackgroundColor");
  SET_COLOR_MACRO(
    "GlobalProperties/TextAnnotationColor",
    QColor::fromRgbF(1, 1, 1),
    "TextAnnotationColor");
  SET_COLOR_MACRO(
    "GlobalProperties/SelectionColor",
    QColor::fromRgbF(1, 0, 1),
    "SelectionColor");
  SET_COLOR_MACRO(
    "GlobalProperties/EdgeColor",
    QColor::fromRgbF(0.0, 0, 0.5),
    "EdgeColor");
}

//-----------------------------------------------------------------------------
/// loads palette i.e. global property values given the name of the palette.
void pqApplicationCore::loadPalette(const QString& paletteName)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes",
    paletteName.toAscii().data());
  if (!prototype)
    {
    qCritical() << "No such palette " << paletteName;
    return;
    }

  vtkSMGlobalPropertiesManager* mgr = this->getGlobalPropertiesManager();
  vtkSMPropertyIterator * iter = mgr->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if (prototype->GetProperty(iter->GetKey()))
      {
      iter->GetProperty()->Copy(
        prototype->GetProperty(iter->GetKey()));
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
/// loads palette i.e. global property values given the name XML state for a
/// palette.
void pqApplicationCore::loadPalette(vtkPVXMLElement* xml)
{
  vtkSMGlobalPropertiesManager* mgr = this->getGlobalPropertiesManager();
  mgr->LoadState(xml, NULL);
}

//-----------------------------------------------------------------------------
/// save the current palette as XML. A new reference is returned, so the
/// caller is responsible for releasing memory i.e. call Delete() on the
/// returned value.
vtkPVXMLElement* pqApplicationCore::getCurrrentPalette()
{
  vtkSMGlobalPropertiesManager* mgr = this->getGlobalPropertiesManager();
  return mgr->SaveState(NULL);
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
void pqApplicationCore::saveState(const QString& filename)
{
  // * Save the Proxy Manager state.
  vtkSMProxyManager::GetProxyManager()->SaveState(filename.toAscii().data());
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqApplicationCore::saveState()
{
  // * Save the Proxy Manager state.
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Eventually proxy manager will save state for each connection separately.
  // For now, we only have one connection, so simply save it.
  return pxm->SaveState();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(const char* filename, pqServer* server)
{
  if (!server || !filename)
    {
    return ;
    }

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();
  this->loadState(parser->GetRootElement(), server);
  parser->Delete();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(
  vtkPVXMLElement* rootElement, pqServer* server)
{
  if (!server || !rootElement)
    {
    return ;
    }

  QList<pqView*> current_views = 
    this->ServerManagerModel->findItems<pqView*>(server);
  foreach (pqView* view, current_views)
    {
    this->ObjectBuilder->destroy(view);
    }

  emit this->aboutToLoadState(rootElement);

  // FIXME: this->LoadingState cannot be relied upon.
  this->LoadingState = true;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->LoadState(rootElement, server->GetConnectionID());
  this->LoadingState = false;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::onStateLoaded(
  vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  emit this->stateLoaded(root, locator);

  pqEventDispatcher::processEventsAndWait(1);

  // This is essential since it's possible that the AnimationTime property on
  // the scenes gets pushed before StartTime and EndTime and as a consequence
  // the scene may not even result in the animation time being set as expected.
  QList<pqAnimationScene*> scenes = 
    this->getServerManagerModel()->findItems<pqAnimationScene*>();
  foreach (pqAnimationScene* scene, scenes)
    {
    scene->getProxy()->UpdateProperty("AnimationTime", 1);
    }
  this->render();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::onStateSaved(vtkPVXMLElement* root)
{
  if (!QApplication::applicationName().isEmpty())
    {
    // Change root element to match the application name.
    QString valid_name =
      QApplication::applicationName().replace(QRegExp("\\W"), "_");
    root->SetName(valid_name.toAscii().data());
    }
  emit this->stateSaved(root);
}

//-----------------------------------------------------------------------------
pqServerResources& pqApplicationCore::serverResources()
{
  if(!this->ServerResources)
    {
    this->ServerResources = new pqServerResources(this);
    this->ServerResources->load(*this->settings());
    }
    
  return *this->ServerResources;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setServerResources(
  pqServerResources* aserverResources)
{
  this->ServerResources = aserverResources;
  if(this->ServerResources)
    {
    this->ServerResources->load(*this->settings());
    }
}

//-----------------------------------------------------------------------------
pqServerStartups& pqApplicationCore::serverStartups()
{
  if(!this->ServerStartups)
    {
    this->ServerStartups = new pqServerStartups(this);
    }
  return *this->ServerStartups;
}

//-----------------------------------------------------------------------------
pqSettings* pqApplicationCore::settings()
{
  if ( !this->Settings )
    {
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    if (options && options->GetDisableRegistry())
      {
      this->Settings = new pqSettings(
        QApplication::organizationName(),
        QApplication::applicationName() + QApplication::applicationVersion() 
        + ".DisabledRegistry", this);
      this->Settings->clear();
      }
    else
      {
      this->Settings = new pqSettings(
        QApplication::organizationName(),
        QApplication::applicationName() + QApplication::applicationVersion(),
        this);
      }
    }
  return this->Settings;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::render()
{
  QList<pqView*> list = 
    this->ServerManagerModel->findItems<pqView*>();
  foreach(pqView* view, list)
    {
    view->render();
    }
}

//-----------------------------------------------------------------------------
pqServer* pqApplicationCore::getActiveServer() const
{
  pqServerManagerModel* smmodel = this->getServerManagerModel();
  return smmodel->getItemAtIndex<pqServer*>(0);
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

//-----------------------------------------------------------------------------
void pqApplicationCore::showOutputWindow()
{
  this->OutputWindow->show();
  this->OutputWindow->raise();
  this->OutputWindow->activateWindow();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::disableOutputWindow()
{
  this->OutputWindowAdapter->setActive(false);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadConfiguration(const QString& filename)
{
  QFile xml(filename);
  if (!xml.open(QIODevice::ReadOnly))
    {
    qCritical() << "Failed to load " << filename;
    return;
    }

  QByteArray dat = xml.readAll();
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();
  if (!parser->Parse(dat.data()))
    {
    xml.close();
    return;
    }

  vtkPVXMLElement* root = parser->GetRootElement();
  
  // Load configuration files for server manager components since they don't
  // listen to Qt signals.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->
    LoadConfiguration(root);
  vtkSMProxyManager::GetProxyManager()->GetWriterFactory()->
    LoadConfiguration(root);

  emit this->loadXML(root);
}

//-----------------------------------------------------------------------------
pqTestUtility* pqApplicationCore::testUtility()
{
  if (!this->TestUtility)
    {
    this->TestUtility = new pqCoreTestUtility(this);
    }
  return this->TestUtility;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadDistributedPlugins(const char* filename)
{
  QString config_file = filename;
  if (!filename)
    {
    config_file = QApplication::applicationDirPath() +  "/.plugins";
#if defined(__APPLE__)
    // for installed applications.
    config_file = QApplication::applicationDirPath() + "/../Support/.plugins";
    if (!QFile::exists(config_file))
      {
      config_file =  QApplication::applicationDirPath() + "/../../../.plugins";
      }
#endif
#if defined(WIN32)
    if (!QFile::exists(config_file))
      {
      // maybe running from the build tree.
      config_file = QApplication::applicationDirPath() + "/../.plugins";
      }
#endif
    }

  vtkSMApplication::GetApplication()->GetPluginManager()->LoadPluginConfigurationXML(
    config_file.toStdString().c_str());
}
