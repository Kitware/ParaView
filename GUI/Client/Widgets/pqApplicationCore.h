/*=========================================================================

   Program:   ParaQ
   Module:    pqApplicationCore.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqWidgetsExport.h"
#include <QObject>

class pq3DWidgetFactory;
class pqApplicationCoreInternal;
class pqPipelineBuilder;
class pqPipelineData;
class pqPipelineSource;
class pqReaderFactory;
class pqRenderModule;
class pqRenderWindowManager;
class pqServer;
class pqServerManagerModel;
class pqUndoStack;
class pqWriterFactory;

/// This class is the crux of the ParaQ application. It creates
/// and manages various managers which are necessary for the PQClient
/// to work with the ServerManager. The functionality implemented by
/// this class itself should be kept minimal. It should typically use
/// delegates to do all the work. This class is merely the toolbox
/// to look for anything of interest. 
/// This class also must be free of actual GUI element i.e. QWidget
/// (and subclasses) probably don't belong here. This will make it
/// it possible for the GUI to change isolated from the core (hopefully). 

class PQWIDGETS_EXPORT pqApplicationCore : public QObject
{
  Q_OBJECT;
public:
  // Get the global instace for the pqApplicationCore.
  static pqApplicationCore* instance();

  pqApplicationCore(QObject* parent=NULL);
  virtual ~pqApplicationCore();


  pqPipelineData* getPipelineData();
  pqServerManagerModel* getServerManagerModel();
  pqUndoStack* getUndoStack();
  pqPipelineBuilder* getPipelineBuilder();
  pqRenderWindowManager* getRenderWindowManager();
  pq3DWidgetFactory* get3DWidgetFactory();
  pqReaderFactory* getReaderFactory();
  pqWriterFactory* getWriterFactory();

  // This will create a source with the given xmlname on the active server. 
  // On success returns
  // pqPipelineSource for the source proxy. The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createSourceOnActiveServer(const QString& xmlname);

  // This will create a filter and connect it to the active source.
  // The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createFilterForActiveSource( const QString& xmlname);

  // This will instantiate and register a compound proxy. A compound proxy
  // definition with the given name must have already been registered with
  // the proxy manager. If the compound proxy needs an input, the active
  // source will be used as the input. 
  pqPipelineSource* createCompoundSource(const QString& name);

  // Utility function to create a reader that reads the file on the 
  // active server. 
  pqPipelineSource* createReaderOnActiveServer( const QString& filename);

  // Use this method to delete the active source. 
  // This involves following operations
  // \li remove all displays from the render modules.
  // \li Break all display connections.
  // \li unregister all displays.
  // \li Unregister the proxy for the \c source.
  // All this is delegated to the pipeline builder.
  // This method can only be called when the active source has no consumers.
  void removeActiveSource();
  void removeSource(pqPipelineSource* source);

  /// returns the active source.
  pqPipelineSource* getActiveSource();

  /// returns the active server.
  pqServer* getActiveServer();

  /// returns the active render module.
  pqRenderModule* getActiveRenderModule();

  /// This method for now renders the Active Render module.
  /// We may want to expose API to render all views or something--need to think 
  /// about it.
  void render();

  /// Returns the number of sources pending displays. This shouldn't even be 
  /// exposed, but for the current release, we want to disable all menus,
  /// if the user has a source waiting a display, hence we expose it.
  int getNumberOfSourcesPendingDisplays();
signals:
  // Fired when the active source changes.
  void activeSourceChanged(pqPipelineSource*);

  // Fired when the active server changes.
  void activeServerChanged(pqServer*);

  // Fired when a source/filter/reader/compound proxy is
  // created without a display.
  void pendingDisplays(bool status);

public slots:
  // Call this slot to set the active source. 
  void setActiveSource(pqPipelineSource*);

  // Call this slot to set the active server. 
  void setActiveServer(pqServer*);

  // Call this slot when accept is called. This method will create
  // displays for any sources/filters that are pending.
  void createPendingDisplays();

protected:
  /// create signal/slot connections between pdata and smModel.
  void connect(pqPipelineData* pdata, pqServerManagerModel* smModel);

private slots:
  // called when a source is removed by the pqServerManagerModel. If
  // the removed source is the active source, we must change it.
  void sourceRemoved(pqPipelineSource*);

private:
  pqApplicationCoreInternal* Internal;
  static pqApplicationCore* Instance;
};

#endif

