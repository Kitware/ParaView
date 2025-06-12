// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqApplicationCore.h"

#include "pqQtConfig.h" // for PARAVIEW_USE_QTHELP

#include <vtksys/SystemTools.hxx>

// Qt includes.
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QRegularExpression>
#include <QSize>
#include <QTemporaryFile>
#include <QtDebug>

// ParaView includes.
#include "pqAnimationScene.h"
#include "pqCoreConfiguration.h"
#include "pqCoreInit.h"
#include "pqCoreTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqEventDispatcher.h"
#include "pqInterfaceTracker.h"
#include "pqLinksModel.h"
#include "pqMainWindowEventManager.h"
#include "pqObjectBuilder.h"
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
#include "vtkCLIOptions.h"
#include "vtkCommand.h"
#include "vtkInitializationHelper.h"
#include "vtkPVFileInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVView.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
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

#include <QLibraryInfo>
#include <QProcessEnvironment>

#include <cassert>

#ifdef PARAVIEW_USE_QTHELP
#include <QHelpEngine>
#endif

//-----------------------------------------------------------------------------
namespace
{
static const QString DEFAULT_SAVE_STATE_FORMAT_KEY = "GeneralSettings.DefaultSaveStateFormat";
};

//-----------------------------------------------------------------------------
class pqApplicationCore::pqInternals
{
public:
  QMap<QString, QPointer<QObject>> RegisteredManagers;
};

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::Instance = nullptr;

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::instance()
{
  return pqApplicationCore::Instance;
}

//-----------------------------------------------------------------------------
pqApplicationCore::pqApplicationCore(int& argc, char** argv, vtkCLIOptions* options /*=nullptr*/,
  bool addStandardArgs /*=true*/, QObject* parentObject /*=nullptr*/)
  : QObject(parentObject)
{
  auto cliOptions = vtk::MakeSmartPointer(options);
  if (!cliOptions)
  {
    cliOptions = vtk::TakeSmartPointer(vtkCLIOptions::New());
  }

  if (addStandardArgs)
  {
    // fill up with pqCoreConfiguration options.
    pqCoreConfiguration::instance()->populateOptions(cliOptions);
  }

  vtkPVView::SetUseGenericOpenGLRenderWindow(true);
  vtkInitializationHelper::SetOrganizationName(QApplication::organizationName().toStdString());
  vtkInitializationHelper::SetApplicationName(QApplication::applicationName().toStdString());
  if (!vtkInitializationHelper::Initialize(
        argc, argv, vtkProcessModule::PROCESS_CLIENT, cliOptions, addStandardArgs))
  {
    // initialization short-circuited. throw exception to exit the application.
    throw pqApplicationCoreExitCode(vtkInitializationHelper::GetExitCode());
  }

  this->constructor();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::constructor()
{
  // Only 1 pqApplicationCore instance can be created.
  assert(pqApplicationCore::Instance == nullptr);
  pqApplicationCore::Instance = this;

  this->UndoStack = nullptr;
  this->RecentlyUsedResourcesList = nullptr;
  this->ServerConfigurations = nullptr;
  this->Settings = nullptr;
  this->HelpEngine = nullptr;

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
  this->PluginManager = nullptr;

  delete this->InterfaceTracker;
  this->InterfaceTracker = nullptr;

  // give chance to save before pqApplicationCore is gone
  delete this->ServerConfigurations;
  this->ServerConfigurations = nullptr;

  delete this->LinksModel;
  this->LinksModel = nullptr;

  delete this->MainWindowEventManager;
  this->MainWindowEventManager = nullptr;

  delete this->ObjectBuilder;
  this->ObjectBuilder = nullptr;

  delete this->ProgressManager;
  this->ProgressManager = nullptr;

  delete this->ServerManagerModel;
  this->ServerManagerModel = nullptr;

  delete this->ServerManagerObserver;
  this->ServerManagerObserver = nullptr;

  delete this->RecentlyUsedResourcesList;
  this->RecentlyUsedResourcesList = nullptr;

  delete this->Settings;
  this->Settings = nullptr;

  if (this->HelpEngine)
  {
#ifdef PARAVIEW_USE_QTHELP
    QString collectionFile = this->HelpEngine->collectionFile();
    delete this->HelpEngine;
    QFile::remove(collectionFile);
#endif
  }
  this->HelpEngine = nullptr;

  // We don't call delete on these since we have already setup parent on these
  // correctly so they will be deleted. It's possible that the user calls delete
  // on these explicitly in which case we end up with segfaults.
  this->UndoStack = nullptr;

  // Delete all children, which clears up all managers etc. before the server
  // manager application is finalized.
  delete this->Internal;

  delete this->TestUtility;

  if (pqApplicationCore::Instance == this)
  {
    pqApplicationCore::Instance = nullptr;
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
    Q_EMIT this->undoStackChanged(stack);
  }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::registerManager(const QString& function, QObject* _manager)
{
  if (this->Internal->RegisteredManagers.contains(function) &&
    this->Internal->RegisteredManagers[function] != nullptr)
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
  QMap<QString, QPointer<QObject>>::iterator iter =
    this->Internal->RegisteredManagers.find(function);
  if (iter != this->Internal->RegisteredManagers.end())
  {
    return iter.value();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::stateFileFormatToExtension(pqApplicationCore::StateFileFormat format)
{
  switch (format)
  {
    case pqApplicationCore::StateFileFormat::Python:
      return "py";
    case pqApplicationCore::StateFileFormat::PVSM:
    default:
      return "pvsm";
  }
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::getDefaultSaveStateFileFormatQString(bool pythonAvailable, bool loading)
{
  unsigned int value = settings()->value(::DEFAULT_SAVE_STATE_FORMAT_KEY, 0).toInt();
  pqApplicationCore::StateFileFormat format = pqApplicationCore::StateFileFormat(value);

  QString pvsmExt = tr("ParaView state file");
  if (loading)
  {
    // .png is only added when loading, as saving .png state files is done using the save screenshot
    // feature
    pvsmExt += QString(" (*.pvsm *.png);;");
  }
  else
  {
    pvsmExt += QString(" (*.pvsm);;");
  }

  QString pyExt;
  if (pythonAvailable)
  {
    pyExt = tr("Python state file") + QString(" (*.py);;");
  }

  QString fileExt;
  // Order matters as first argument is default
  if (format == pqApplicationCore::StateFileFormat::PVSM)
  {
    fileExt = pvsmExt + pyExt;
  }
  else
  {
    fileExt = pyExt + pvsmExt;
  }
  fileExt += tr("All Files") + QString(" (*)");

  return fileExt;
}

//-----------------------------------------------------------------------------
bool pqApplicationCore::saveState(const QString& filename, vtkTypeUInt32 location)
{
  // * Save the Proxy Manager state.
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  Q_EMIT this->aboutToWriteState(filename);

  return pxm->SaveXMLState(filename.toUtf8().data(), location);
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

  Q_EMIT this->aboutToReadState(filename);

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
  QList<QPointer<pqProxy>> to_destroy;
  Q_FOREACH (pqProxy* proxy, proxies)
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
  Q_FOREACH (pqProxy* cur, to_destroy)
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

  Q_EMIT aboutToReadState(filename);

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename.toUtf8().data());
  parser->Parse();
  this->loadStateIncremental(parser->GetRootElement(), server, loader);
  parser->Delete();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::loadStateIncremental(
  vtkPVXMLElement* rootElement, pqServer* server, vtkSMStateLoader* loader)
{
  Q_EMIT this->aboutToLoadState(rootElement);

  // TODO: this->LoadingState cannot be relied upon.
  this->LoadingState = true;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->LoadXMLState(rootElement, loader);
  this->LoadingState = false;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  Q_EMIT this->stateLoaded(root, locator);

  pqEventDispatcher::processEventsAndWait(1);

  // This is essential since it's possible that the AnimationTime property on
  // the scenes gets pushed before StartTime and EndTime and as a consequence
  // the scene may not even result in the animation time being set as expected.
  QList<pqAnimationScene*> scenes = this->getServerManagerModel()->findItems<pqAnimationScene*>();
  Q_FOREACH (pqAnimationScene* scene, scenes)
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
    QString valid_name = QApplication::applicationName().replace(QRegularExpression("\\W"), "_");
    root->SetName(valid_name.toUtf8().data());
  }
  Q_EMIT this->stateSaved(root);
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
QString pqApplicationCore::getSettingFileBaseName()
{
  QString fileBaseName = QApplication::applicationName();

  if (this->VersionedSettings)
  {
    fileBaseName += QApplication::applicationVersion();
  }

  const bool disableSettings = vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry();
  if (disableSettings)
  {
    fileBaseName += "-dr";
  }

  return fileBaseName;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::useVersionedSettings(bool use)
{
  this->VersionedSettings = use;
}

//-----------------------------------------------------------------------------
pqSettings* pqApplicationCore::settings()
{
  if (!this->Settings)
  {
    vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Loading client settings");

    // First look for site settings and set them as SystemScope,
    // for automatic fallback
    const QString settingsOrg = QApplication::organizationName();
    const QString iniFileBaseName = this->getSettingFileBaseName();
    // QSettings enforce this directory hierarchy: ini file is under an "organization" subdir.
    // https://doc.qt.io/qt-5/qsettings.html#platform-specific-notes
    const QString iniRelativePath = settingsOrg + "/" + iniFileBaseName + ".ini";
    const QString systemDirPath = pqCoreUtilities::findInApplicationDirectories(iniRelativePath);
    if (!systemDirPath.isEmpty() && QFile::exists(systemDirPath))
    {
      vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        "using system scope settings under " << systemDirPath.toStdString() << ", containing "
                                             << iniRelativePath.toStdString());
      QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, systemDirPath);
    }
    else
    {
      vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        << "system scope settings not found " << iniRelativePath.toStdString());
    }

    // Create settings with UserScope priority.
    auto settings = new pqSettings(
      QSettings::IniFormat, QSettings::UserScope, settingsOrg, iniFileBaseName, this);

    const bool disable_settings = vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry();
    if (disable_settings || settings->value("pqApplicationCore.DisableSettings", false).toBool())
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading of Qt settings skipped (disabled).");
      settings->clear();
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading Qt user settings from '%s'",
        settings->fileName().toUtf8().data());
    }
    // now settings are ready!

    this->Settings = settings;
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
  Q_FOREACH (pqView* view, list)
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
  Q_FOREACH (pqServer* server, this->getServerManagerModel()->findChildren<pqServer*>())
  {
    server->session()->PreDisconnection();
  }

  // As tempting as it is to connect this slot to
  // aboutToQuit() signal, it doesn't work since that signal is not
  // fired until the event loop exits, which doesn't happen until animation
  // stops playing.
  QList<pqAnimationScene*> scenes = this->getServerManagerModel()->findItems<pqAnimationScene*>();
  Q_FOREACH (pqAnimationScene* scene, scenes)
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

  this->updateAvailableReadersAndWriters();

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

  Q_EMIT this->loadXML(root);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::updateAvailableReadersAndWriters()
{
  // Load configuration files for server manager components since they don't
  // listen to Qt signals.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->UpdateAvailableReaders();
  vtkSMProxyManager::GetProxyManager()->GetWriterFactory()->UpdateAvailableWriters();
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

//-----------------------------------------------------------------------------
QHelpEngine* pqApplicationCore::helpEngine()
{
#ifdef PARAVIEW_USE_QTHELP
  if (!this->HelpEngine)
  {
    QTemporaryFile tFile;
    tFile.open();
    QString collectionFileName = tFile.fileName() + ".qhc";
    this->HelpEngine = new QHelpEngine(collectionFileName, this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    this->HelpEngine->setReadOnly(false);
#endif
    this->connect(
      this->HelpEngine, SIGNAL(warning(const QString&)), SLOT(onHelpEngineWarning(const QString&)));
    if (!this->HelpEngine->setupData())
    {
      qWarning() << tr("paraview", "Cannot set up help engine from %1").arg(collectionFileName);
    }
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
    Q_FOREACH (const QString& filename, help_files)
    {
      QString qch_file =
        QString(":/%1/Documentation/%2").arg(QApplication::applicationName()).arg(filename);
      this->registerDocumentation(qch_file);
    }
    bool success = this->HelpEngine->setupData();
    if (!success)
    {
      qWarning() << "Failed to setup help engine data.";
    }
  }
#endif

  return this->HelpEngine;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::registerDocumentation(const QString& filename)
{
#ifdef PARAVIEW_USE_QTHELP
  QHelpEngine* engine = this->helpEngine();

  // QHelpEngine doesn't like files from resource space. So we create a local
  // file and use that.
  QTemporaryFile* localFile = QTemporaryFile::createNativeFile(filename);
  if (localFile)
  {
    // localFile has autoRemove ON by default, so the file will be deleted with
    // the application quits.
    localFile->setParent(engine);
    auto localFileName = localFile->fileName();
    bool success = engine->registerDocumentation(localFileName);
    if (!success)
    {
      qWarning() << tr("Failed to register documentation in %1 via file: %2")
                      .arg(filename)
                      .arg(localFile->fileName());
    }
  }
  else
  {
    engine->registerDocumentation(filename);
  }
#else
  (void)filename;
#endif
}

//-----------------------------------------------------------------------------
void pqApplicationCore::generalSettingsChanged()
{
  if (auto pvsettings = vtkPVGeneralSettings::GetInstance())
  {
    pqDoubleLineEdit::RealNumberNotation realNotation =
      static_cast<pqDoubleLineEdit::RealNumberNotation>(
        pvsettings->GetRealNumberDisplayedNotation());
    int realPrecision = pvsettings->GetRealNumberDisplayedShortestAccuratePrecision()
      ? QLocale::FloatingPointShortest
      : pvsettings->GetRealNumberDisplayedPrecision();
    pqDoubleLineEdit::setGlobalPrecisionAndNotation(realPrecision, realNotation);
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
  Q_EMIT this->clientEnvironmentDone();
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::getInterfaceLanguage()
{
  QProcessEnvironment options = QProcessEnvironment::systemEnvironment();
  if (options.contains("PV_TRANSLATIONS_LOCALE"))
  {
    return options.value("PV_TRANSLATIONS_LOCALE");
  }
  else if (this->settings()->contains("GeneralSettings.InterfaceLanguage"))
  {
    return this->settings()->value("GeneralSettings.InterfaceLanguage").toString();
  }
  return QString("en");
}

//-----------------------------------------------------------------------------
QString pqApplicationCore::getTranslationsPathFromInterfaceLanguage(QString prefix, QString locale)
{
  if (locale.isEmpty())
  {
    return QString();
  }
  QList<QDir> paths;
  QProcessEnvironment options = QProcessEnvironment::systemEnvironment();
  if (options.contains("PV_TRANSLATIONS_DIR"))
  {
    for (QString path : options.value("PV_TRANSLATIONS_DIR").split(":"))
    {
      paths.append(QDir(path));
    }
  }
  QString translationsPath(vtkPVFileInformation::GetParaViewTranslationsDirectory().c_str());
  /* PV_TRANSLATIONS_DIR `override` translationsPath's qm files,
    thus translationsPath has to be added lastly */
  paths.append(translationsPath);
  for (QDir directory : paths)
  {
    for (QFileInfo fileInfo : directory.entryInfoList(QDir::Files))
    {
      if (fileInfo.suffix() == "qm")
      {
        QLocale fileLocale(
          fileInfo.completeBaseName().mid(fileInfo.completeBaseName().indexOf("_") + 1));
        if (fileInfo.completeBaseName().startsWith(prefix + "_") && fileLocale == QLocale(locale))
        {
          return fileInfo.absoluteDir().absolutePath();
        }
      }
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
QTranslator* pqApplicationCore::getQtTranslations(QString prefix, QString locale)
{
  QTranslator* translator = new QTranslator(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QString qtQmPath(QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#else
  QString qtQmPath(QLibraryInfo::path(QLibraryInfo::TranslationsPath));
#endif
  bool qtLoaded = translator->load(QLocale(locale), prefix, "_", qtQmPath);
  if (qtLoaded)
  {
    return translator;
  }
  qtQmPath = this->getTranslationsPathFromInterfaceLanguage(prefix, locale);
  qtLoaded = translator->load(QLocale(locale), prefix, "_", qtQmPath);
  if (qtLoaded)
  {
    return translator;
  }
  qWarning() << QString("Could not load a %1 translation file with associated locale %2")
                  .arg(prefix, locale);
  return nullptr;
}
