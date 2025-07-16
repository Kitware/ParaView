// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqApplicationCore_h
#define pqApplicationCore_h

#include "pqCoreModule.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTranslator>
#include <exception> // for std::exception

class pqInterfaceTracker;
class pqLinksModel;
class pqMainWindowEventManager;
class pqObjectBuilder;
class pqPipelineSource;
class pqPluginManager;
class pqProgressManager;
class pqRecentlyUsedResourcesList;
class pqServer;
class pqServerConfigurationCollection;
class pqServerManagerModel;
class pqServerManagerObserver;
class pqServerResource;
class pqSettings;
class pqTestUtility;
class pqUndoStack;
class QApplication;
class QHelpEngine;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkSMStateLoader;
class vtkCLIOptions;

class PQCORE_EXPORT pqApplicationCoreExitCode : public std::exception
{
  int Code = 0;

public:
  pqApplicationCoreExitCode(int ecode)
    : Code(ecode)
  {
  }
  int code() const { return this->Code; }
};

/**
 * This class is the crux of the ParaView application. It creates
 * and manages various managers which are necessary for the ParaView-based
 * client to work with the ServerManager.
 * For clients based of the pqCore library,
 * simply instantiate this pqApplicationCore after QApplication initialization
 * and then create your main window etc. like a standard Qt application. You can then
 * use the facilities provided by pqCore such as the pqObjectBuilder,
 * pqUndoStack etc. in your application. After that point.
 */
class PQCORE_EXPORT pqApplicationCore : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  // Get the global instance for the pqApplicationCore.
  static pqApplicationCore* instance();

  /**
   * Initializes the ParaView application engine. `vtkCLIOptions` may be provide
   * if the application wants to customize command line argument processing. By
   * default all standard ParaView-specific command line arguments will be
   * supported. To avoid that, simply pass `addStandardArgs=false`.
   *
   * @note This constructor may raise `pqApplicationCoreExitCode` exception
   * if the initialization process was short circuited and the application should
   * simply quit.
   *
   * @note If `options` is nullptr, an vtkCLIOptions instance will be
   * created and used. It is also setup to accept extra / unknown arguments
   * without raising errors.
   */
  pqApplicationCore(int& argc, char** argv, vtkCLIOptions* options = nullptr,
    bool addStandardArgs = true, QObject* parent = nullptr);

  /**
   * Get the Object Builder. Object Buider must be used
   * to create complex objects such as sources, filters,
   * readers, views, displays etc.
   */
  pqObjectBuilder* getObjectBuilder() const { return this->ObjectBuilder; }

  /**
   * Set/Get the application's central undo stack. By default no undo stack is
   * provided. Applications must set on up as required.
   */
  void setUndoStack(pqUndoStack* stack);
  pqUndoStack* getUndoStack() const { return this->UndoStack; }

  /**
   * Custom Applications may need use various "managers"
   * All such manager can be registered with the pqApplicationCore
   * so that that can be used by other components of the
   * application. Registering with pqApplicationCore gives easy
   * access to these managers from the application code. Note
   * that custom applications are not required to register managers.
   * However certain optional components of the pqCore may
   * expect some managers.
   * Only one manager can be registered for a \c function.
   */
  void registerManager(const QString& function, QObject* manager);

  /**
   * Returns a manager for a particular function, if any.
   */
  //. \sa registerManager
  QObject* manager(const QString& function);

  /**
   * Unregisters a manager for a particular function, if any.
   */
  void unRegisterManager(const QString& function);

  /**
   * provides access to the help engine. The engine is created the first time
   * this method is called.
   */
  QHelpEngine* helpEngine();

  /**
   * QHelpEngine doesn't like filenames in resource space. This method creates
   * a temporary file for the resource and registers that with the QHelpEngine.
   */
  void registerDocumentation(const QString& filename);

  /**
   * ServerManagerObserver observer the vtkSMProxyManager
   * for changes to the server manager and fires signals on
   * certain actions such as registeration/unregistration of proxies
   * etc. Returns the ServerManagerObserver used by the application.
   */
  pqServerManagerObserver* getServerManagerObserver() { return this->ServerManagerObserver; }

  /**
   * ServerManagerModel is the representation of the ServerManager
   * using pqServerManagerModelItem subclasses. It makes it possible to
   * explore the ServerManager with ease by separating proxies based
   * on their functionality/type.
   */
  pqServerManagerModel* getServerManagerModel() const { return this->ServerManagerModel; }

  /**
   * Locates the interface tracker for the application. pqInterfaceTracker is
   * used to locate all interface implementations typically loaded from
   * plugins.
   */
  pqInterfaceTracker* interfaceTracker() const { return this->InterfaceTracker; }

  /**
   * pqLinksModel is the model used to keep track of proxy/property links
   * maintained by vtkSMProxyManager.
   * TODO: It may be worthwhile to investigate if we even need a global
   * pqLinksModel. All the information is already available in
   * vtkSMProxyManager.
   */
  pqLinksModel* getLinksModel() const { return this->LinksModel; }

  /**
   * pqMainWindowManager manages signals called for main window events.
   */
  pqMainWindowEventManager* getMainWindowEventManager() const
  {
    return this->MainWindowEventManager;
  }

  /**
   * pqPluginManager manages all functionality associated with loading plugins.
   */
  pqPluginManager* getPluginManager() const { return this->PluginManager; }

  /**
   * ProgressManager is the manager that streamlines progress.
   */
  pqProgressManager* getProgressManager() const { return this->ProgressManager; }

  /**
   * Provides access to the test utility.
   */
  virtual pqTestUtility* testUtility();

  /**
   * Returns the set of recently-used resources i.e. data files and state
   * files.
   */
  pqRecentlyUsedResourcesList& recentlyUsedResources();

  /**
   * Returns the collection of server configurations known. Server
   * configurations have information about connecting to different servers.
   */
  pqServerConfigurationCollection& serverConfigurations();

  /**
   * Get the application settings.
   * This uses a user settings file (created as needed) and falls back
   * to an installed settings file for options not found in the user file.
   *
   * When "--disable-registry (--dr)" is specified, a "XXX-dr.ini" alternative file
   * is used instead of the usual config, so nothing is read/write on usual files.
   * Then clearSettings is called.
   *
   * @see clearSettings(), useVersionedSettings(), pqCoreUtilities::findInApplicationDirectories()
   */
  pqSettings* settings();

  /**
   * Clears the user settings. Since various UI components that only
   * read settings at creation time may get out of sync, it's best
   * to warn the user to restart the application.
   *
   * Any changes made to pqSettings after calling this method will be lost and
   * will not get restored. If that's not desired, see `QSettings::clear`.
   *
   * Site settings are still present, though.
   */
  void clearSettings();

  /**
   * Enum to capture possible Save State file formats.
   */
  enum class StateFileFormat : unsigned int
  {
    PVSM = 0,
    Python = 1
  };

  /**
   * Return the extension for the given format
   * See StateFileFormat.
   * Default to pvsm
   */
  static QString stateFileFormatToExtension(pqApplicationCore::StateFileFormat format);

  /**
   * Return the QString to populate the pqFileDialog with available file formats for saving/loading
   * State. Takes into account the default state file format setting.
   */
  QString getDefaultSaveStateFileFormatQString(bool pythonAvailable, bool loading);

  /**
   * Set versioned settings mode.
   * If true, the setting file name includes application version numbers.
   * Should be set before the first call to settings(): later calls will
   * not be taken into account.
   *
   * Default is false.
   */
  void useVersionedSettings(bool use);

  /**
   * Save the ServerManager state to a XML element.
   */
  vtkPVXMLElement* saveState();

  /**
   * Save the ServerManager state to a file.
   * Return true if the operation succeeded otherwise return false.
   */
  bool saveState(const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Loads the ServerManager state. Emits the signal
   * stateLoaded() on loading state successfully.
   */
  void loadState(vtkPVXMLElement* root, pqServer* server, vtkSMStateLoader* loader = nullptr);

  /**
   * Load state xml from a file. The filename can be a Qt resource file,
   * besides regular filesystem files (refer to QFile documentation for more
   * information on Qt resource system).
   */
  void loadState(const char* filename, pqServer* server, vtkSMStateLoader* loader = nullptr);

  /**
   * Loads state from an in-memory buffer.
   */
  void loadStateFromString(
    const char* xmlcontents, pqServer* server, vtkSMStateLoader* loader = nullptr);

  void clearViewsForLoadingState(pqServer* server);

  /**
   * Same as loadState() except that it doesn't clear the current visualization
   * state.
   */
  void loadStateIncremental(
    vtkPVXMLElement* root, pqServer* server, vtkSMStateLoader* loader = nullptr);
  void loadStateIncremental(
    const QString& filename, pqServer* server, vtkSMStateLoader* loader = nullptr);

  /**
   * Set the loading state flag
   */
  void setLoadingState(bool value) { this->LoadingState = value; };

  /**
   * Check to see if its in the process of loading a state
   * Reliance on this flag is chimerical since we cannot set this ivar when
   * state file is being loaded from python shell.
   */
  bool isLoadingState() { return this->LoadingState; };

  /**
   * returns the active server is any.
   */
  pqServer* getActiveServer() const;

  /**
   * returns a path to a directory containing a translation binary file located
   * in a path specified by the PV_TRANSLATIONS_DIR environment variable or in
   * the resources directory.
   * The binary file returned is the first one to have a name matching the locale
   * given in the user settings.
   * @param prefix: The prefix the filename should contain.
   * @param locale: The locale the filename suffix should contain.
   */
  QString getTranslationsPathFromInterfaceLanguage(QString prefix, QString locale);

  /**
   * returns a QTranslator with a loaded qm file. The qm file can come either
   * from the Qt's translation directory or from the ParaView's shared
   * directory.
   * @param prefix: The prefix the filename should contain.
   * @param locale: The locale the filename suffix should contain.
   */
  QTranslator* getQtTranslations(QString prefix, QString locale);

  /**
   * returns interface language in use in a locale code form.
   * It is first read from PV_INTERFACE_LANGUAGE environment
   * variable if defined, then recovered from user settings.
   * If not set, return `en`.
   */
  QString getInterfaceLanguage();

  /**
   * Destructor.
   */
  ~pqApplicationCore() override;

  /**
   * INTERNAL. Do not use.
   */
  void _paraview_client_environment_complete();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * Applications calls this to ensure that any cleanup is performed correctly.
   */
  void prepareForQuit();

  /**
   * Calls QCoreApplication::quit().
   */
  void quit();

  ///@{
  /**
   * Load configuration xml. This results in firing of the loadXML() signal
   * which different components that support configuration catch and process to
   * update their behavior.
   * This also update available readers and writers.
   */
  void loadConfiguration(const QString& filename);
  void loadConfigurationXML(const char* xmldata);
  ///@}

  /**
   * Update the available readers and writers using the factories
   */
  void updateAvailableReadersAndWriters();

  /**
   * Renders all windows
   */
  void render();

Q_SIGNALS:
  /**
   * Fired before a state file, either python or XML, is written. Note that writing
   * can still fail at this point.
   */
  void aboutToWriteState(QString filename);

  /**
   * Fired before a state file, either python or XML, is read. Note that loading can
   * still fail at this point.
   */
  void aboutToReadState(QString filename);

  /**
   * Fired before a state xml is being loaded. One can add slots for this signal
   * and modify the fired xml-element as part of pre-processing before
   * attempting to load the state xml. Note that never attempt to connect to
   * signal in a delayed fashion i.e using Qt::QueuedConnection etc. since the
   * \c root will be destroyed.
   */
  void aboutToLoadState(vtkPVXMLElement* root);

  /**
   * Fired when a state file is loaded successfully.
   * GUI components that may have state saved in the XML state file must listen
   * to this signal and handle process the XML to update their state.
   */
  void stateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);

  /**
   * Fired to save state xml. Components that need to save XML state should
   * listen to this signal and add their XML elements to the root. DO NOT MODIFY
   * THE ROOT besides adding new children.
   */
  void stateSaved(vtkPVXMLElement* root);

  /**
   * Fired when the undo stack is set.
   */
  void undoStackChanged(pqUndoStack*);

  /**
   * Fired on loadConfiguration().
   */
  void loadXML(vtkPVXMLElement*);

  /**
   * Fired when the filter menu state needs to be manually updated
   */
  void forceFilterMenuRefresh();

  /**
   * Fired when master changed. true if current user is master, false otherwise.
   */
  void updateMasterEnableState(bool);

  /**
   * Fired when the ParaView Client infrastructure has completed setting up the
   * environment.
   */
  void clientEnvironmentDone();

protected Q_SLOTS:
  void onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);
  void onStateSaved(vtkPVXMLElement* root);
  void onHelpEngineWarning(const QString&);

private Q_SLOTS:
  /**
   * called when vtkPVGeneralSettings::GetInstance() fired
   * `vtkCommand::ModifiedEvent`. We update pqDoubleLineEdit's global precision
   * settings.
   */
  void generalSettingsChanged();

protected:
  bool LoadingState;

  pqLinksModel* LinksModel;
  pqObjectBuilder* ObjectBuilder;
  pqInterfaceTracker* InterfaceTracker;
  pqMainWindowEventManager* MainWindowEventManager;
  pqPluginManager* PluginManager;
  pqProgressManager* ProgressManager;
  pqServerManagerModel* ServerManagerModel;
  pqServerManagerObserver* ServerManagerObserver;
  pqUndoStack* UndoStack;
  pqRecentlyUsedResourcesList* RecentlyUsedResourcesList;
  pqServerConfigurationCollection* ServerConfigurations;
  pqSettings* Settings;
  QHelpEngine* HelpEngine;
  QPointer<pqTestUtility> TestUtility;

private:
  Q_DISABLE_COPY(pqApplicationCore)

  class pqInternals;
  pqInternals* Internal;
  static pqApplicationCore* Instance;
  void constructor();

  /**
   * Get Setting file name.
   * This use application name and optional suffix (version and -dr)
   */
  QString getSettingFileBaseName();

  bool VersionedSettings = false;
};

#endif
