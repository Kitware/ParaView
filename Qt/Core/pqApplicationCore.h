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
#ifndef __pqApplicationCore_h
#define __pqApplicationCore_h

#include "pqCoreModule.h"
#include <QObject>
#include <QPointer>

class pq3DWidgetFactory;
class pqDisplayPolicy;
class pqInterfaceTracker;
class pqLinksModel;
class pqObjectBuilder;
class pqOptions;
class pqOutputWindow;
class pqOutputWindowAdapter;
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
class QStringList;
class vtkPVXMLElement;
class vtkSMProxyLocator;

/// This class is the crux of the ParaView application. It creates
/// and manages various managers which are necessary for the ParaView-based
/// client to work with the ServerManager.
/// For clients based of the pqCore library,
/// simply instantiate this pqApplicationCore after QApplication initialization
/// and then create your main window etc. like a standard Qt application. You can then
/// use the facilities provided by pqCore such as the pqObjectBuilder,
/// pqUndoStack etc. in your application. After that point.
class PQCORE_EXPORT pqApplicationCore : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  // Get the global instace for the pqApplicationCore.
  static pqApplicationCore* instance();

  /// Preferred constructor. Initializes the server-manager engine and sets up
  /// the core functionality. If application supports special command line
  /// options, pass an instance of pqOptions subclass to the constructor,
  /// otherwise a new instance of pqOptions with standard ParaView command line
  /// options will be created.
  pqApplicationCore(int& argc, char** argv, pqOptions* options=0, QObject* parent=0);

  /// Provides access to the command line options object.
  pqOptions* getOptions() const
    { return this->Options; }

  /// Get the Object Builder. Object Buider must be used
  /// to create complex objects such as sources, filters,
  /// readers, views, displays etc.
  pqObjectBuilder* getObjectBuilder() const
    { return this->ObjectBuilder; }

  /// Set/Get the application's central undo stack. By default no undo stack is
  /// provided. Applications must set on up as required.
  void setUndoStack(pqUndoStack* stack);
  pqUndoStack* getUndoStack() const
    { return this->UndoStack; }

  /// Custom Applications may need use various "managers"
  /// All such manager can be registered with the pqApplicationCore
  /// so that that can be used by other components of the
  /// application. Registering with pqApplicationCore gives easy
  /// access to these managers from the application code. Note
  /// that custom applications are not required to register managers.
  /// However certain optional components of the pqCore may
  /// expect some managers.
  /// Only one manager can be registered for a \c function.
  void registerManager(const QString& function, QObject* manager);

  /// Returns a manager for a particular function, if any.
  //. \sa registerManager
  QObject* manager(const QString& function);

  /// Unregisters a manager for a particular function, if any.
  void unRegisterManager(const QString& function);

  /// provides access to the help engine. The engine is created the first time
  /// this method is called.
  QHelpEngine* helpEngine();

  /// QHelpEngine doesn't like filenames in resource space. This method creates
  /// a temporary file for the resource and registers that with the QHelpEngine.
  void registerDocumentation(const QString& filename);

  /// ServerManagerObserver observer the vtkSMProxyManager
  /// for changes to the server manager and fires signals on
  /// certain actions such as registeration/unregistration of proxies
  /// etc. Returns the ServerManagerObserver used by the application.
  pqServerManagerObserver* getServerManagerObserver()
    { return this->ServerManagerObserver; }

  /// ServerManagerModel is the representation of the ServerManager
  /// using pqServerManagerModelItem subclasses. It makes it possible to
  /// explore the ServerManager with ease by separating proxies based
  /// on their functionality/type.
  pqServerManagerModel* getServerManagerModel() const
    { return this->ServerManagerModel; }

  pq3DWidgetFactory* get3DWidgetFactory() const
    { return this->WidgetFactory; }

  /// Locates the interface tracker for the application. pqInterfaceTracker is
  /// used to locate all interface implementations typically loaded from
  /// plugins.
  pqInterfaceTracker* interfaceTracker() const
    { return this->InterfaceTracker; }

  /// pqLinksModel is the model used to keep track of proxy/property links
  /// maintained by vtkSMProxyManager.
  /// TODO: It may be worthwhile to investigate if we even need a global
  /// pqLinksModel. All the information is already available in
  /// vtkSMProxyManager.
  pqLinksModel* getLinksModel() const
    { return this->LinksModel; }

  /// pqPluginManager manages all functionality associated with loading plugins.
  pqPluginManager* getPluginManager() const
    { return this->PluginManager; }

  /// ProgressManager is the manager that streamlines progress.
  pqProgressManager* getProgressManager() const
    { return this->ProgressManager; }

  //// Returns the display policy instance used by the application.
  //// pqDisplayPolicy defines the policy for creating representations
  //// for sources.
  pqDisplayPolicy* getDisplayPolicy() const
    { return this->DisplayPolicy; }

  /// Returns the output window.
  pqOutputWindowAdapter* outputWindowAdapter()
    { return this->OutputWindowAdapter; }

  pqOutputWindow* outputWindow()
  { return this->OutputWindow; }

  /// It is possible to change the display policy used by
  /// the application. Used to change the active display
  /// policy. The pqApplicationCore takes over the ownership of the display policy.
  void setDisplayPolicy(pqDisplayPolicy* dp);

  /// Provides access to the test utility.
  virtual pqTestUtility* testUtility();

  /// Returns the set of recently-used resources i.e. data files and state
  /// files.
  pqRecentlyUsedResourcesList& recentlyUsedResources();

  /// Returns the collection of server configurations known. Server
  /// configurations have information about connecting to different servers.
  pqServerConfigurationCollection& serverConfigurations();

  /// Get the application settings.
  pqSettings* settings();

  /// Save the ServerManager state.
  vtkPVXMLElement* saveState();
  void saveState(const QString& filename);

  /// Loads the ServerManager state. Emits the signal
  /// stateLoaded() on loading state successfully.
  void loadState(vtkPVXMLElement* root, pqServer* server);
  void loadState(const char* filename, pqServer* server);

  /// Check to see if its in the process of loading a state
  /// Reliance on this flag is chimerical since we cannot set this ivar when
  /// state file is  being loaded from python shell.
  bool isLoadingState(){return this->LoadingState;};

  /// returns the active server is any.
  pqServer* getActiveServer() const;

  /// Called to load the configuration xml bundled with the application the
  /// lists the plugins that the application is aware by default. If no filename
  /// is specified, {executable-path}/.plugins is loaded.
  void loadDistributedPlugins(const char* filename=0);

  /// Destructor.
  virtual ~pqApplicationCore();
public slots:

  /// Applications calls this to ensure
  /// that any cleanup is performed correctly.
  void prepareForQuit();

  /// Calls QCoreApplication::quit().
  void quit();

  /// Causes the output window to be shown.
  void showOutputWindow();

  /// Load configuration xml. This results in firing of the loadXML() signal
  /// which different components that support configuration catch and process to
  /// update their behavior.
  void loadConfiguration(const QString& filename);
  void loadConfigurationXML(const char* xmldata);

  /// Renders all windows
  void render();

signals:
  /// Fired before a state xml is being loaded. One can add slots for this signal
  /// and modify the fired xml-element as part of pre-processing before
  /// attempting to load the state xml. Note that never attempt to connect to
  /// signal in a delayed fashion i.e using Qt::QueuedConnection etc. since the
  /// \c root will be destroyed.
  void aboutToLoadState(vtkPVXMLElement* root);

  /// Fired when a state file is loaded successfully.
  /// GUI components that may have state saved in the XML state file must listen
  /// to this signal and handle process the XML to update their state.
  void stateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);

  /// Fired to save state xml. Components that need to save XML state should
  /// listen to this signal and add their XML elements to the root. DO NOT MODIFY
  /// THE ROOT besides adding new children.
  void stateSaved(vtkPVXMLElement* root);

  /// Fired when the undo stack is set.
  void undoStackChanged(pqUndoStack*);

  /// Fired on loadConfiguration().
  void loadXML(vtkPVXMLElement*);

  /// Fired when the filter menu state needs to be manually updated
  void forceFilterMenuRefresh();

  /// Fired when master changed. true if current user is master, false otherwise.
  void updateMasterEnableState(bool);

protected slots:
  void onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);
  void onStateSaved(vtkPVXMLElement* root);

protected:
  bool LoadingState;

  pqOutputWindow* OutputWindow;
  pqOutputWindowAdapter* OutputWindowAdapter;
  pqOptions* Options;

  pq3DWidgetFactory* WidgetFactory;
  pqDisplayPolicy* DisplayPolicy;
  pqLinksModel* LinksModel;
  pqObjectBuilder* ObjectBuilder;
  pqInterfaceTracker* InterfaceTracker;
  pqPluginManager* PluginManager;
  pqProgressManager* ProgressManager;
  pqServerManagerModel* ServerManagerModel;
  pqServerManagerObserver* ServerManagerObserver;
  pqUndoStack* UndoStack;
  pqRecentlyUsedResourcesList *RecentlyUsedResourcesList;
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
  void createOutputWindow();
};

#endif

