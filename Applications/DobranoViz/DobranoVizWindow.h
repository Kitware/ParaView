/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _DobranoVizWindow_h
#define _DobranoVizWindow_h

#include <pqVariableType.h>
#include <QMainWindow>
#include <vtkIOStream.h>

class pqObjectInspectorWidget;
class pqPipelineData;
class pqPipelineListWidget;
class pqPipelineServer;
class pqPipelineWindow;
class pqServer;
class pqSMAdaptor;
class pqSourceProxyInfo;
class pqMultiViewManager;
class pqMultiViewFrame;

class QVTKWidget;
class vtkSMRenderModuleProxy;
class vtkEventQtSlotConnect;
class vtkUnstructuredGrid;
class vtkSMProxy;

class QAction;
class QDockWidget;
class QToolBar;
class QTreeView;
class QTabWidget;

/// Provides the main window for the ParaQ application
class DobranoVizWindow :
        public QMainWindow
{
  Q_OBJECT

public:
  DobranoVizWindow();
  virtual ~DobranoVizWindow();

  virtual bool eventFilter(QObject* watched, QEvent* e);

  /// Compares the contents of the window with the given reference image, returns true iff they "match" within some tolerance
  bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);

signals:
  void serverChanged(pqServer*);
  
private:
  void setServer(pqServer* Server);

  pqServer* CurrentServer;
  QToolBar* PropertyToolbar;
  pqMultiViewManager* MultiViewManager;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  pqPipelineData *Pipeline;
  QMenu* SourcesMenu;
  QMenu* FiltersMenu;
  QMenu* ToolsMenu;
  pqObjectInspectorWidget *Inspector;
  QDockWidget *InspectorDock;
  QAction *InspectorDockAction;
  pqPipelineListWidget *PipelineList;
  QDockWidget *PipelineDock;
  QAction *PipelineDockAction;
  QDockWidget *HistogramDock;
  QDockWidget *LineChartDock;
  QAction *HistogramDockAction;
  QAction *LineChartDockAction;
  pqMultiViewFrame* ActiveView;
  QDockWidget *ElementInspectorDock;
  QAction *ElementDockAction;
  QToolBar* CompoundProxyToolBar;
  QToolBar* VariableSelectorToolBar;
  QToolBar* VCRControlsToolBar;

  pqSourceProxyInfo* ProxyInfo;
  vtkEventQtSlotConnect* VTKConnector;
  vtkSMProxy* CurrentProxy;

private slots:
  
  void onNewQVTKWidget(pqMultiViewFrame* parent);
  void onDeleteQVTKWidget(pqMultiViewFrame* parent);
  void onFrameActive(QWidget*);

  void onFileNew();
  void onFileNew(pqServer* Server);
  void onFileOpen();
  void onFileOpen(pqServer* Server);
  void onFileOpen(const QStringList& Files);
  void onFileOpenServerState();
  void onFileOpenServerState(pqServer* Server);
  void onFileOpenServerState(const QStringList& Files);
  void onFileSaveServerState();
  void onFileSaveServerState(const QStringList& Files);
  void onFileSaveScreenshot();
  void onFileSaveScreenshot(const QStringList& Files);
  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();
  void onUpdateSourcesFiltersMenu(pqServer*);
  void onUpdateWindows();
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);
  void onOpenLinkEditor();
  void onOpenCompoundFilterWizard();

  void onNewSelections(vtkSMProxy* p, vtkUnstructuredGrid* selections);
  void onCompoundProxyAdded(const QString&, const QString&);
  void onCreateCompoundProxy(QAction*);
  
  void onProxySelected(vtkSMProxy*);
  void onVariableChanged(pqVariableType, const QString&);
  
  void onRecordTest();
  void onRecordTest(const QStringList& Files);
  void onPlayTest();
  void onPlayTest(const QStringList& Files);
  
  void onAddServer(pqPipelineServer *server);
  void onRemoveServer(pqPipelineServer *server);
  void onAddWindow(pqPipelineWindow *window);

  void onFirstTimeStep();
  void onPreviousTimeStep();
  void onNextTimeStep();
  void onLastTimeStep();

private:
  void cleanUpWindow(QVTKWidget *window);
};

#endif // !_DobranoVizWindow_h

