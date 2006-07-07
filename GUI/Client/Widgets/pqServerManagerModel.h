/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModel.h

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

=========================================================================*/
#ifndef __pqServerManagerModel_h
#define __pqServerManagerModel_h

#include <QObject>
#include "pqWidgetsExport.h"
#include "vtkType.h" // neded for vtkIdType.

class vtkSMProxy;
class vtkSMRenderModuleProxy;
class QVTKWidget;

class pqPipelineDisplay;
class pqPipelineSource;
class pqRenderModule;
class pqServer;
class pqServerManagerModelInternal;
class pqServerManagerModelItem;

// pqServerManagerModel is the model for the Server Manager.
// All the pipelines in the Server Manager need a GUI representation
// to obtain additional information about the connections etc etc.
// This class collects that. This is merely representation of all the
// information available in the Server Manager in a more GUI friendly 
// way. Simplicity is the key here.
// REFERENCE: This class takes functionality from older versions of pqPipelineModel
// with the objective to separate the Model and the view-specific model.
// NOTE: This class makes no distinction between a source and a filter
// so when every we say a source, we mean source as well as a filter unless
// otherwise stated.
class PQWIDGETS_EXPORT pqServerManagerModel : public QObject
{
  Q_OBJECT
public:
  pqServerManagerModel(QObject* parent=NULL);
  virtual ~pqServerManagerModel();

  // Avoid using this, this will be take out soon.
  static pqServerManagerModel* instance();

  /// Given a vtkSMProxy get the pqServer on which the proxy exists.
  pqServer* getServerForSource(vtkSMProxy*);
  pqServer* getServerForSource(pqPipelineSource*);

  // Given a process module ConnectionID, get pqServer for it.
  pqServer* getServer(vtkIdType connectionId);

  // Get the number of server connections available.
  unsigned int getNumberOfServers();

  // Get the number of sources. Eventually, we may want this 
  // to take a server argument and returns the number of sources
  // on the server.
  unsigned int getNumberOfSources();

  /// Given the name for a Source, get the pqServer on which the
  //// source exists.
  // pqServer* getServerForSource(QString name);

  /// Given a vtkSMProxy get the pqServer representation for that proxy,
  /// if one exists.
  pqPipelineSource* getPQSource(vtkSMProxy*);

  /// Given a vtkSMProxy for a display get the pqPipelineDisplay
  /// object for it, if one exists.
  pqPipelineDisplay* getPQDisplay(vtkSMProxy*);

  /// Returns the number of render modules
  int getNumberOfRenderModules();

  /// Given an index, returns a render module.
  pqRenderModule* getRenderModule(int idx);

  /// Given a render module proxy get the pqRenderModule representation
  /// for it.
  pqRenderModule* getRenderModule(vtkSMRenderModuleProxy*);
  pqRenderModule* getRenderModule(QVTKWidget*);

  /// Book end events for removing a server.
  void beginRemoveServer(pqServer *server);
  void endRemoveServer();

  /// Returns a list of servers.
  QList<pqServer*> getServers();

  /// Returns a list of sources on the particular server.
  QList<pqPipelineSource*> getSources(pqServer* server);

public slots:
  /// Call when a  new vtkSMProxy is registered with the proxy manager
  /// under the "sources" group.
  /// This will create a PQ representation for the proxy.
  void onAddSource(QString name, vtkSMProxy* source);

  /// Call when a vtkSMProxy is unregistered from the "sources" group.
  /// This will clean up the PQ representation for the proxy
  /// and sever any connections to the proxy.
  void onRemoveSource(QString name);
  void onRemoveSource(vtkSMProxy*);

  /// Call when a new connection is made. For now we pass
  /// a pqServer as an argument. Would like this to be a vtkIdType.
  void onAddServer(vtkIdType cid);
  void onAddServer(pqServer*);

  /// Call when a new connection is closed/aborted. For now we pass
  /// a pqServer as an argument. Would like this to be a vtkIdType.
  void onRemoveServer(pqServer*);
  void onRemoveServer(vtkIdType cid);

  /// Call when a new render module is registered/unrgistered.
  void onAddRenderModule(QString name, vtkSMRenderModuleProxy* rm);
  void onRemoveRenderModule(vtkSMRenderModuleProxy* rm);

  /// Call when a display proxy is registered/unregistered.
  void onAddDisplay(QString name, vtkSMProxy* display);
  void onRemoveDisplay(vtkSMProxy* display);

signals:
  // Fired when a new connection is created on the vtkProcessModule.
  // a new pqServer encapsulator is created for it (thanks to
  // onAddSource) and then this signal is emitted.
  void serverAdded(pqServer* server);

  /// Fired when beginRemoveServer is called.
  void aboutToRemoveServer(pqServer* server);

  /// Fired when endRemoveServer is called.
  void finishedRemovingServer();

  // When a connection is disconneted on the vtkProcessModule,
  // this event is fired before the pqServer object is destroyed.
  // It is not safe to access the actual vtkProcessModule connection 
  // in this event, since it is already closed and unavailable.
  void serverRemoved(pqServer* server);

  // Fired when a source/filter/compoundProxy is registered.
  void sourceAdded(pqPipelineSource* source);

  // Fired when a source/filter/compoundproxy is unregistered.
  void sourceRemoved(pqPipelineSource* source);

  /// Fired when a connection between two pqPipelineSources is created.
  void connectionAdded(pqPipelineSource* in, pqPipelineSource* out);

  /// Fired when a connection between tow pqPipelineSources is broken.
  void connectionRemoved(pqPipelineSource* in, pqPipelineSource* out);

  /// Fired when a render module becomes available. The handler of this
  /// signal must set the pqRenderModule's parent QWidget at first 
  /// opportunity.
  void renderModuleAdded(pqRenderModule* rm);

  /// Fired when a render module is gone for ever.
  void renderModuleRemoved(pqRenderModule* rm);

  /// Fired when the name of an item changes.
  void nameChanged(pqServerManagerModelItem *item);

  /// Fired when the displays for an item changes.
  void sourceDisplayChanged(pqPipelineSource *source,
      pqPipelineDisplay *display);

private slots:
  /// Used to map a server name change to the name changed signal.
  void updateServerName();

  /// Used to map a display visibility change to the visibility
  /// changed signal.
  void updateDisplayVisibility(bool);

private:
  pqServerManagerModelInternal* Internal;
  static pqServerManagerModel* Instance;
};

#endif

