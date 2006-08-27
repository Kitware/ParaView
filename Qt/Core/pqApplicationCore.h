/*=========================================================================

   Program: ParaView
   Module:    pqApplicationCore.h

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
#ifndef __pqApplicationCore_h
#define __pqApplicationCore_h

#include "pqCoreExport.h"
#include <QObject>

class pq3DWidgetFactory;
class pqApplicationCoreInternal;
class pqPendingDisplayManager;
class pqPipelineBuilder;
class pqPipelineSource;
class pqReaderFactory;
class pqRenderModule;
class pqServer;
class pqServerManagerModel;
class pqServerManagerObserver;
class pqServerManagerSelectionModel;
class pqServerResource;
class pqServerResources;
class pqServerStartups;
class pqSettings;
class pqUndoStack;
class pqWriterFactory;
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
  Q_OBJECT;
public:
  // Get the global instace for the pqApplicationCore.
  static pqApplicationCore* instance();

  pqApplicationCore(QObject* parent=NULL);
  virtual ~pqApplicationCore();


  pqServerManagerObserver* getServerManagerObserver();
  pqServerManagerObserver* getPipelineData()
    {return this->getServerManagerObserver(); }
  pqServerManagerModel* getServerManagerModel();
  pqUndoStack* getUndoStack();
  pqPipelineBuilder* getPipelineBuilder();
  pq3DWidgetFactory* get3DWidgetFactory();
  pqReaderFactory* getReaderFactory();
  pqWriterFactory* getWriterFactory();
  pqPendingDisplayManager* getPendingDisplayManager();

  // Returns the server manager selection model.
  pqServerManagerSelectionModel* getSelectionModel();


  // Use this method to delete the active source. 
  // This involves following operations
  // \li remove all displays from the render modules.
  // \li Break all display connections.
  // \li unregister all displays.
  // \li Unregister the proxy for the \c source.
  // All this is delegated to the pipeline builder.
  // This method can only be called when the active source has no consumers.
  void removeSource(pqPipelineSource* source);

  void removeServer(pqServer *server);

  /// Save the ServerManager state.
  void saveState(vtkPVXMLElement* root);

  /// Loads the ServerManager state. Emits the signal
  /// stateLoaded() on loading state successfully.
  void loadState(vtkPVXMLElement* root, pqServer* server, 
    vtkSMStateLoader* loader=NULL);

  /// Returns the set of available server resources
  pqServerResources& serverResources();
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

  pqServer* createServer(const pqServerResource& resource);
  
  pqPipelineSource* createFilterForSource(const QString& xmlname,
                                          pqPipelineSource* input);
  
  pqPipelineSource* createSourceOnServer(const QString& xmlname, 
                                         pqServer* server);

  /// Create a compound proxy on a server with an input.
  /// If the compound proxy doesn't have an "Input" property,
  /// input is ignored.
  pqPipelineSource* createCompoundFilter(const QString& xmlname, 
                                         pqServer* server,
                                         pqPipelineSource* input);

  pqPipelineSource* createReaderOnServer(const QString& filename, 
                                         pqServer* server);

  /// Renders all windows
  void render();

signals:
  // Fired to enable or disable progress bar.
  void enableProgress(bool enable);

  // Fired with the actual progress value.
  void progress(const QString&, int);

  // Fired when a state file is loaded successfully.
  void stateLoaded();

  // HACK
  void sourceRemoved(pqPipelineSource*);
  
  void sourceCreated(pqPipelineSource*);

protected:
  /// create signal/slot connections between pdata and smModel.
  void connect(pqServerManagerObserver* pdata, pqServerManagerModel* smModel);

protected:

  friend class pqProcessModuleGUIHelper;

  void prepareProgress()
    { emit this->enableProgress(true); }

  void cleanupPendingProgress()
    { emit this->enableProgress(false); }

  void sendProgress(const char* name, int value)
    {
    emit this->progress(QString(name), value);
    }


private:
  pqApplicationCoreInternal* Internal;
  static pqApplicationCore* Instance;
};

#endif

