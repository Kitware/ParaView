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

#include "pqCoreExport.h"
#include <QObject>

class pq3DWidgetFactory;
class pqApplicationCoreInternal;
class pqDisplayPolicy;
class pqLinksModel;
class pqLookmarkManagerModel;
class pqLookupTableManager;
class pqObjectBuilder;
class pqPendingDisplayManager;
class pqPipelineSource;
class pqPluginManager;
class pqProgressManager;
class pqRenderViewModule;
class pqServer;
class pqServerManagerModel;
class pqServerManagerObserver;
class pqServerManagerSelectionModel;
class pqServerResource;
class pqServerResources;
class pqServerStartups;
class pqSettings;
class pqUndoStack;
class vtkPVXMLElement;
class vtkSMStateLoader;

/// This class is the crux of the ParaView application. It creates
/// and manages various managers which are necessary for the PQClient
/// to work with the ServerManager. The functionality implemented by
/// this class itself should be kept minimal. It should typically use
/// delegates to do all the work. This class is merely the toolbox
/// to look for anything of interest. 
/// This class also must be free of actual GUI element i.e. QWidget
/// (and subclasses) probably don't belong here. This will make it
/// it possible for the GUI to change isolated from the core (hopefully). 

class PQCORE_EXPORT pqApplicationCore : public QObject
{
  Q_OBJECT
public:
  // Get the global instace for the pqApplicationCore.
  static pqApplicationCore* instance();

  pqApplicationCore(QObject* parent=NULL);
  virtual ~pqApplicationCore();

  /// Get the Object Builder. Object Buider must be used
  /// to create complex objects such as sources, filters,
  /// readers, views, displays etc.
  pqObjectBuilder* getObjectBuilder() const;

  /// Set/Get the application undo stack.
  /// No undo stack is set up by default. The application
  /// must create and set one if it should support undo/redo
  /// operations.
  /// I'd really like the application core not reference the
  /// the undo stack at all. However, time and again we have 
  /// some widget somewhere in the GUI that needs access to the undo
  /// stack. It's a pain to provide the undo stack to evety such deep
  /// widget, hence we provide this access location. 
  /// Everyone using getUndoStack() must handle the case
  /// when this method returns NULL.
  void setUndoStack(pqUndoStack* stack);
  pqUndoStack* getUndoStack() const;

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

  /// ServerManagerObserver observer the vtkSMProxyManager
  /// for changes to the server manager and fires signals on
  /// certain actions such as registeration/unregistration of proxies
  /// etc. Returns the ServerManagerObserver used by the application.
  pqServerManagerObserver* getServerManagerObserver();

  /// ServerManagerModel is the representation of the ServerManager
  /// using pqServerManagerModelItem subclasses. It makes it possible to
  /// explore the ServerManager with ease by separating proxies based 
  /// on their functionality/type.
  pqServerManagerModel* getServerManagerModel();

  pq3DWidgetFactory* get3DWidgetFactory();
  pqLinksModel* getLinksModel();
  pqPluginManager* getPluginManager();

  /// ProgressManager is the manager that streamlines progress.
  pqProgressManager* getProgressManager() const;

  // Returns the display policy instance used by the application.
  // pqDisplayPolicy defines the policy for creating displays
  // given a (source,view) pair.
  pqDisplayPolicy* getDisplayPolicy() const;

  // It is possible to change the display policy used by
  // the application. Used to change the active display
  // policy.
  void setDisplayPolicy(pqDisplayPolicy*);

  // Returns the server manager selection model.
  pqServerManagerSelectionModel* getSelectionModel();

  // Set/Get the lookup table manager. 
  void setLookupTableManager(pqLookupTableManager*);
  pqLookupTableManager* getLookupTableManager() const;

  /// Save the ServerManager state.
  void saveState(vtkPVXMLElement* root);

  /// Loads the ServerManager state. Emits the signal
  /// stateLoaded() on loading state successfully.
  void loadState(vtkPVXMLElement* root, pqServer* server, 
    vtkSMStateLoader* loader=NULL);

  /// Returns the set of available server resources
  pqServerResources& serverResources();
  /// Set server resources
  void setServerResources(pqServerResources* serverResources);
  
  /// Returns an object that can start remote servers
  pqServerStartups& serverStartups();

  /// Get the application settings.
  pqSettings* settings();

  /// Set/get the application name for the application settings.
  void setApplicationName(const QString&);
  QString applicationName();

  /// Set/get the organization name for the application settngs.
  void setOrganizationName(const QString&);
  QString organizationName();

  /// Renders all windows
  void render();

  /// Set the application specific state loader to use 
  /// while loading states, if any. This is used
  /// only when loadState is called with loader=NULL.
  void setStateLoader(vtkSMStateLoader* loader);

  // Check to see if its in the process of loading a state
  bool isLoadingState(){return this->LoadingState;};

public slots:
  /// Called QCoreApplication::quit().
  /// Applications should use this method instead of directly
  /// calling QCoreApplication::quit() since this ensures
  /// that any cleanup is performed correctly.
  void quit();

signals:
  // Fired when a state file is loaded successfully.
  void stateLoaded();

protected:

  friend class pqProcessModuleGUIHelper;

  /// called to start accepting progress.
  void prepareProgress();

  /// called to stop accepting progress.
  void cleanupPendingProgress();

  /// called to udpate progress.
  void sendProgress(const char* name, int value);

  bool LoadingState;

private:
  pqApplicationCoreInternal* Internal;
  static pqApplicationCore* Instance;
};

#endif

