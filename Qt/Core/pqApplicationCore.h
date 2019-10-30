/*=========================================================================

   Program: ParaView
   Module:    pqApplicationCore.h

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
#ifndef pqApplicationCore_h
#define pqApplicationCore_h

#include "pqCoreModule.h"
#include "vtkPVConfig.h" // for PARAVIEW_USE_QTHELP
#include <QObject>
#include <QPointer>

class pqInterfaceTracker;
class pqLinksModel;
class pqMainWindowEventManager;
class pqObjectBuilder;
class pqOptions;
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
#ifdef PARAVIEW_USE_QTHELP
class QHelpEngine;
#endif
class QStringList;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkSMStateLoader;

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
  * Preferred constructor. Initializes the server-manager engine and sets up
  * the core functionality. If application supports special command line
  * options, pass an instance of pqOptions subclass to the constructor,
  * otherwise a new instance of pqOptions with standard ParaView command line
  * options will be created.
  */
  pqApplicationCore(int& argc, char** argv, pqOptions* options = 0, QObject* parent = 0);

  /**
  * Provides access to the command line options object.
  */
  pqOptions* getOptions() const { return this->Options; }

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

#ifdef PARAVIEW_USE_QTHELP
  /**
  * provides access to the help engine. The engine is created the first time
  * this method is called.
  */
  QHelpEngine* helpEngine();
#endif

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
  */
  pqSettings* settings();

  /**
   * Clears the settings. Since various UI components that only
   * read settings at creation time may get out of sync, it's best
   * to warn the user to restart the application.
   *
   * Any changes made to pqSettings after calling this method will be lost and
   * will not get restored. If that's not desired, see `QSettings::clear`.
   */
  void clearSettings();

  /**
  * Save the ServerManager state.
  */
  vtkPVXMLElement* saveState();
  void saveState(const QString& filename);

  /**
  * Loads the ServerManager state. Emits the signal
  * stateLoaded() on loading state successfully.
  */
  void loadState(vtkPVXMLElement* root, pqServer* server, vtkSMStateLoader* loader = NULL);

  /**
  * Load state xml from a file. The filename can be a Qt resource file,
  * besides regular filesystem files (refer to QFile documentation for more
  * information on Qt resource system).
  */
  void loadState(const char* filename, pqServer* server, vtkSMStateLoader* loader = NULL);

  /**
  * Loads state from an in-memory buffer.
  */
  void loadStateFromString(
    const char* xmlcontents, pqServer* server, vtkSMStateLoader* loader = NULL);

  void clearViewsForLoadingState(pqServer* server);

  /**
  * Same as loadState() except that it doesn't clear the current visualization
  * state.
  */
  void loadStateIncremental(
    vtkPVXMLElement* root, pqServer* server, vtkSMStateLoader* loader = NULL);
  void loadStateIncremental(
    const QString& filename, pqServer* server, vtkSMStateLoader* loader = NULL);

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
  * Destructor.
  */
  ~pqApplicationCore() override;

  /**
   * INTERNAL. Do not use.
   */
  void _paraview_client_environment_complete();

public slots:

  /**
  * Applications calls this to ensure
  * that any cleanup is performed correctly.
  */
  void prepareForQuit();

  /**
  * Calls QCoreApplication::quit().
  */
  void quit();

  /**
  * Load configuration xml. This results in firing of the loadXML() signal
  * which different components that support configuration catch and process to
  * update their behavior.
  */
  void loadConfiguration(const QString& filename);
  void loadConfigurationXML(const char* xmldata);

  /**
  * Renders all windows
  */
  void render();

signals:
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

protected slots:
  void onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);
  void onStateSaved(vtkPVXMLElement* root);
  void onHelpEngineWarning(const QString&);

private slots:
  /**
   * called when vtkPVGeneralSettings::GetInstance() fired
   * `vtkCommand::ModifiedEvent`. We update pqDoubleLineEdit's global precision
   * settings.
   */
  void generalSettingsChanged();

protected:
  bool LoadingState;

  pqOptions* Options;
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
#ifdef PARAVIEW_USE_QTHELP
  QHelpEngine* HelpEngine;
#endif
  QPointer<pqTestUtility> TestUtility;

private:
  Q_DISABLE_COPY(pqApplicationCore)

  class pqInternals;
  pqInternals* Internal;
  static pqApplicationCore* Instance;
  void constructor();
};

#endif
