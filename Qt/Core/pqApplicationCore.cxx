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
#ifdef PARAVIEW_USE_QTHELP
#include <QHelpEngine>
#endif
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSize>
#include <QTemporaryFile>
#include <QtDebug>

// ParaView includes.
#include "pqAnimationScene.h"
#include "pqCoreInit.h"
#include "pqCoreTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqEventDispatcher.h"
#include "pqInterfaceTracker.h"
#include "pqLinksModel.h"
#include "pqMainWindowEventManager.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProgressManager.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerConfigurationCollection.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSettings.h"
#include "pqStandardServerManagerModelInterface.h"
#include "pqUndoStack.h"
#include "pqXMLUtil.h"
#include "vtkCommand.h"
#include "vtkInitializationHelper.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVView.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMWriterFactory.h"
#include "vtkSmartPointer.h"

#include <cassert>

//-----------------------------------------------------------------------------
class pqApplicationCore::pqInternals
{
public:
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
pqApplicationCore::pqApplicationCore(
  int& argc, char** argv, pqOptions* options, QObject* parentObject)
  : QObject(parentObject)
{
  vtkPVView::SetUseGenericOpenGLRenderWindow(true);

  vtkSmartPointer<pqOptions> defaultOptions;
  if (!options)
  {
    defaultOptions = vtkSmartPointer<pqOptions>::New();
    options = defaultOptions;
  }
  this->Options = options;

  vtkInitializationHelper::SetOrganizationName(QApplication::organizationName().toStdString());
  vtkInitializationHelper::SetApplicationName(QApplication::applicationName().toStdString());
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT, options);
  this->constructor();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::constructor()
{
  // Only 1 pqApplicationCore instance can be created.
  assert(pqApplicationCore::Instance == NULL);
  pqApplicationCore::Instance = this;

  this->UndoStack = NULL;
  this->RecentlyUsedResourcesList = NULL;
  this->ServerConfigurations = NULL;
  this->Settings = NULL;
#ifdef PARAVIEW_USE_QTHELP
  this->HelpEngine = NULL;
#endif

  // initialize statics in case we're a static library
  pqCoreInit();

  this->Internal = new pqInternals();

  // *  Create pqServerManagerObserver first. This is the vtkSMProxyManager observer.
  this->ServerManagerObserver = new pqServerManagerObserver(this);

  // *  Make signal-slot connections between ServerManagerObserver and ServerManagerModel.
  this->ServerManagerModel = new pqServerManagerModel(this->ServerManagerObserver, this);

  // *  Create the pqObjectBuilder. This is used to create pipeline objects.
  this->ObjectBuilder = new pqObjectBuilder(this);

  this->InterfaceTracker = new pqInterfaceTracker(this);

  this->PluginManager = new pqPluginManager(this);

  // * Create various factories.
  this->ProgressManager = new pqProgressManager(this);

  // add standard server manager model interface
  this->InterfaceTracker->addInterface(
    new pqStandardServerManagerModelInterface(this->InterfaceTracker));

  this->LinksModel = new pqLinksModel(this);

  this->LoadingState = false;
  QObject::connect(this->ServerManagerObserver,
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this,
    SLOT(onStateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(this->ServerManagerObserver, SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(onStateSaved(vtkPVXMLElement*)));
  // CAUTION: We do not want to connect this slot to aboutToQuit()
  //  => See prepareForQuit() for more details.
  QObject::connect(
    QCoreApplication::instance(), SIGNAL(lastWindowClosed()), this, SLOT(prepareForQuit()));

  // this has to happen after the construction of pqInterfaceTracker since if
  // the plugin initialization code itself may request access to  the interface
  // tracker.
  this->InterfaceTracker->initialize();

  if (auto pvsettings = vtkPVGeneralSettings::GetInstance())
  {
    // pqDoubleLineEdit's global precision is linked to parameters in
    // vtkPVGeneralSettings. Let's set that up here.
    pqCoreUtilities::connect(
      pvsettings, vtkCommand::ModifiedEvent, this, SLOT(generalSettingsChanged()));
  }

  // * Set up the manager for converting main window events to signals.
  this->MainWindowEventManager = new pqMainWindowEventManager(this);
}

//-----------------------------------------------------------------------------
pqApplicationCore::~pqApplicationCore()
{
  // Ensure that startup plugins get a chance to cleanup before pqApplicationCore is gone.
  delete this->PluginManager;
  this->PluginManager = 0;

  delete this->InterfaceTracker;
  this->InterfaceTracker = 0;

  // give chance to save before pqApplicationCore is gone
  delete this->ServerConfigurations;
  this->ServerConfigurations = 0;

  delete this->LinksModel;
  this->LinksModel = 0;

  delete this->MainWindowEventManager;
  this->MainWindowEventManager = 0;

  delete this->ObjectBuilder;
  this->ObjectBuilder = 0;

  delete this->ProgressManager;
  this->ProgressManager = 0;

  delete this->ServerManagerModel;
  this->ServerManagerModel = 0;

  delete this->ServerManagerObserver;
  this->ServerManagerObserver = 0;

  delete this->RecentlyUsedResourcesList;
  this->RecentlyUsedResourcesList = 0;

  delete this->Settings;
  this->Settings = 0;

#ifdef PARAVIEW_USE_QTHELP
  if (this->HelpEngine)
  {
    QString collectionFile = this->HelpEngine->collectionFile();
    delete this->HelpEngine;
    QFile::remove(collectionFile);
  }
  this->HelpEngine = NULL;
#endif

  // We don't call delete on these since we have already setup parent on these
  // correctly so they will be deleted. It's possible that the user calls delete
  // on these explicitly in which case we end up with segfaults.
  this->UndoStack = 0;

  // Delete all children, which clears up all managers etc. before the server
  // manager application is finalized.
  delete this->Internal;

  delete this->TestUtility;

  if (pqApplicationCore::Instance == this)
  {
    pqApplicationCore::Instance = 0;
  }

  vtkInitializationHelper::Finalize();
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
void pqApplicationCore::registerManager(const QString& function, QObject* _manager)
{
  if (this->Internal->RegisteredManagers.contains(function) &&
    this->Internal->RegisteredManagers[function] != 0)
  {
    qDebug() << "Replacing existing manager for function : " << function;
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
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  pxm->SaveXMLState(filename.toLocal8Bit().data());
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqApplicationCore::saveState()
{
  // * Save the Proxy Manager state.
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  // Eventually proxy manager will save state for each connection separately.
  // For now, we only have one connection, so simply save it.
  return pxm->SaveXMLState();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(const char* filename, pqServer* server, vtkSMStateLoader* loader)
{
  if (!server || !filename)
  {
    return;
  }

  QFile qfile(filename);
  if (qfile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    this->loadStateFromString(qfile.readAll().data(), server, loader);
  }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadStateFromString(
  const char* xmlcontents, pqServer* server, vtkSMStateLoader* loader)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  if (xmlcontents && parser->Parse(xmlcontents))
  {
    this->loadState(parser->GetRootElement(), server, loader);
  }
  parser->Delete();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::clearViewsForLoadingState(pqServer* server)
{
  // BUG #12398:
  // This change was added to prevent VisTrails from recording unwanted events.
  // We disable recording view deletion in Undo/Stack
  // In anycase, the stack will be cleared, why bother recording something...
  BEGIN_UNDO_EXCLUDE();
  QList<pqProxy*> proxies = this->ServerManagerModel->findItems<pqProxy*>(server);
  QList<QPointer<pqProxy> > to_destroy;
  foreach (pqProxy* proxy, proxies)
  {
    pqView* view = qobject_cast<pqView*>(proxy);
    if (view)
    {
      to_destroy.push_back(view);
    }
    else if (proxy->getSMGroup() == "layouts")
    {
      to_destroy.push_back(proxy);
    }
  }
  foreach (pqProxy* cur, to_destroy)
  {
    pqView* view = qobject_cast<pqView*>(cur);
    if (view)
    {
      this->ObjectBuilder->destroy(view);
    }
    else if (cur)
    {
      this->ObjectBuilder->destroy(cur);
    }
  }
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadState(
  vtkPVXMLElement* rootElement, pqServer* server, vtkSMStateLoader* loader)
{
  if (!server || !rootElement)
  {
    return;
  }

  this->clearViewsForLoadingState(server);
  this->loadStateIncremental(rootElement, server, loader);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadStateIncremental(
  const QString& filename, pqServer* server, vtkSMStateLoader* loader)
{
  if (!server || filename.isEmpty())
  {
    return;
  }
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename.toLocal8Bit().data());
  parser->Parse();
  this->loadStateIncremental(parser->GetRootElement(), server, loader);
  parser->Delete();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadStateIncremental(
  vtkPVXMLElement* rootElement, pqServer* server, vtkSMStateLoader* loader)
{
  emit this->aboutToLoadState(rootElement);

  // TODO: this->LoadingState cannot be relied upon.
  this->LoadingState = true;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->LoadXMLState(rootElement, loader);
  this->LoadingState = false;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  emit this->stateLoaded(root, locator);

  pqEventDispatcher::processEventsAndWait(1);

  // This is essential since it's possible that the AnimationTime property on
  // the scenes gets pushed before StartTime and EndTime and as a consequence
  // the scene may not even result in the animation time being set as expected.
  QList<pqAnimationScene*> scenes = this->getServerManagerModel()->findItems<pqAnimationScene*>();
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
    QString valid_name = QApplication::applicationName().replace(QRegExp("\\W"), "_");
    root->SetName(valid_name.toLocal8Bit().data());
  }
  emit this->stateSaved(root);
}

//-----------------------------------------------------------------------------
pqRecentlyUsedResourcesList& pqApplicationCore::recentlyUsedResources()
{
  if (!this->RecentlyUsedResourcesList)
  {
    this->RecentlyUsedResourcesList = new pqRecentlyUsedResourcesList(this);
    this->RecentlyUsedResourcesList->load(*this->settings());
  }

  return *this->RecentlyUsedResourcesList;
}

//-----------------------------------------------------------------------------
pqServerConfigurationCollection& pqApplicationCore::serverConfigurations()
{
  if (!this->ServerConfigurations)
  {
    this->ServerConfigurations = new pqServerConfigurationCollection(this);
  }
  return *this->ServerConfigurations;
}

//-----------------------------------------------------------------------------
pqSettings* pqApplicationCore::settings()
{
  if (!this->Settings)
  {
    auto options = pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
    bool disable_settings = (options && options->GetDisableRegistry());

    const QString settingsOrg = QApplication::organizationName();
    const QString settingsApp =
      QApplication::applicationName() + QApplication::applicationVersion();

    // for the app name, we use a "-dr" suffix is disable_settings is true.
    const QString suffix(disable_settings ? "-dr" : "");

    auto settings = new pqSettings(
      QSettings::IniFormat, QSettings::UserScope, settingsOrg, settingsApp + suffix, this);
    if (disable_settings || settings->value("pqApplicationCore.DisableSettings", false).toBool())
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading of Qt settings skipped (disabled).");
      settings->clear();
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading Qt settings from '%s'",
        settings->fileName().toLocal8Bit().data());
    }
    // now settings are ready!

    this->Settings = settings;

    vtkProcessModuleAutoMPI::SetEnableAutoMPI(
      this->Settings->value("GeneralSettings.EnableAutoMPI").toBool());
    vtkProcessModuleAutoMPI::SetNumberOfCores(
      this->Settings->value("GeneralSettings.AutoMPILimit").toInt());
  }
  return this->Settings;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::clearSettings()
{
  auto settings = this->settings();
  settings->clear();

  // this will ensure that the settings won't get restored.
  settings->setValue("pqApplicationCore.DisableSettings", true);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::render()
{
  QList<pqView*> list = this->ServerManagerModel->findItems<pqView*>();
  foreach (pqView* view, list)
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
void pqApplicationCore::prepareForQuit()
{
  foreach (pqServer* server, this->getServerManagerModel()->findChildren<pqServer*>())
  {
    server->session()->PreDisconnection();
  }

  // As tempting as it is to connect this slot to
  // aboutToQuit() signal, it doesn't work since that signal is not
  // fired until the event loop exits, which doesn't happen until animation
  // stops playing.
  QList<pqAnimationScene*> scenes = this->getServerManagerModel()->findItems<pqAnimationScene*>();
  foreach (pqAnimationScene* scene, scenes)
  {
    scene->pause();
  }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::quit()
{
  this->prepareForQuit();
  QCoreApplication::instance()->quit();
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
  this->loadConfigurationXML(dat.data());
  xml.close();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadConfigurationXML(const char* xmldata)
{
  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();

  if (!parser->Parse(xmldata))
  {
    return;
  }

  // Now, the reader/writer factories cannot be initialized until after a
  // session has been created. So what do we do? Do we save the xml for
  // processing everytime the session startsup?
  vtkPVXMLElement* root = parser->GetRootElement();

  // Load configuration files for server manager components since they don't
  // listen to Qt signals.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->UpdateAvailableReaders();
  vtkSMProxyManager::GetProxyManager()->GetWriterFactory()->UpdateAvailableWriters();

  // Give a warning that if there is ParaViewReaders or ParaViewWriters in root
  // that it has been changed and people should change their code accordingly.
  if (strcmp(root->GetName(), "ParaViewReaders") == 0)
  {
    vtkGenericWarningMacro("Readers have been changed such that the GUI definition is not needed."
      << " This should now be specified in the Hints section of the XML definition.");
  }
  else if (strcmp(root->GetName(), "ParaViewWriters") == 0)
  {
    vtkGenericWarningMacro("Writers have been changed such that the GUI definition is not needed."
      << " This should now be specified in the Hints section of the XML definition.");
  }

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
void pqApplicationCore::onHelpEngineWarning(const QString& msg)
{
  qWarning() << msg;
}

#ifdef PARAVIEW_USE_QTHELP
//-----------------------------------------------------------------------------
QHelpEngine* pqApplicationCore::helpEngine()
{
  if (!this->HelpEngine)
  {
    QTemporaryFile tFile;
    tFile.open();
    this->HelpEngine = new QHelpEngine(tFile.fileName() + ".qhc", this);
    this->connect(
      this->HelpEngine, SIGNAL(warning(const QString&)), SLOT(onHelpEngineWarning(const QString&)));
    this->HelpEngine->setupData();
    // register the application's qch file. An application specific qch file can
    // be compiled into the executable in the build_paraview_client() cmake
    // function. If this file is provided, then that gets registered as
    // :/${application_name}/Documentation/${qch-filename}.
    // Locate all such registered resources and register them with the help
    // engine.
    QDir dir(QString(":/%1/Documentation").arg(QApplication::applicationName()));
    QStringList help_files;
    if (dir.exists())
    {
      QStringList filters;
      filters << "*.qch";
      help_files = dir.entryList(filters, QDir::Files);
    }
    foreach (const QString& filename, help_files)
    {
      QString qch_file =
        QString(":/%1/Documentation/%2").arg(QApplication::applicationName()).arg(filename);
      this->registerDocumentation(qch_file);
    }
    this->HelpEngine->setupData();
  }

  return this->HelpEngine;
}
#endif

//-----------------------------------------------------------------------------
void pqApplicationCore::registerDocumentation(const QString& filename)
{
#ifdef PARAVIEW_USE_QTHELP
  QHelpEngine* engine = this->helpEngine();

  // QHelpEngine doesn't like files from resource space. So we create a local
  // file and use that.
  QTemporaryFile* localFile = QTemporaryFile::createLocalFile(filename);
  if (localFile)
  {
    // localFile has autoRemove ON by default, so the file will be deleted with
    // the application quits.
    localFile->setParent(engine);
    engine->registerDocumentation(localFile->fileName());
  }
  else
  {
    engine->registerDocumentation(filename);
  }
#endif
}

//-----------------------------------------------------------------------------
void pqApplicationCore::generalSettingsChanged()
{
  if (auto pvsettings = vtkPVGeneralSettings::GetInstance())
  {
    pqDoubleLineEdit::setGlobalPrecisionAndNotation(
      pvsettings->GetRealNumberDisplayedPrecision(),
      static_cast<pqDoubleLineEdit::RealNumberNotation>(
        pvsettings->GetRealNumberDisplayedNotation()));
  }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::_paraview_client_environment_complete()
{
  static bool Initialized = false;
  if (Initialized)
  {
    return;
  }

  Initialized = true;
  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "clientEnvironmentDone");
  emit this->clientEnvironmentDone();
}
