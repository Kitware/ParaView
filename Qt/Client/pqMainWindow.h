/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqMainWindow_h
#define _pqMainWindow_h

#include <QMainWindow>
#include <vtkIOStream.h>

class pqObjectInspectorWidget;
class pqPipelineData;
class pqPipelineListWidget;
class pqRefreshToolbar;
class pqServer;
class pqSMAdaptor;
class pqMultiViewManager;
class pqMultiViewFrame;

class vtkSMSourceProxy;
class QVTKWidget;
class vtkSMRenderModuleProxy;

class QAction;
class QDockWidget;
class QToolBar;
class QTreeView;
class QTabWidget;

/// Provides the main window for the ParaQ application
class pqMainWindow :
        public QMainWindow
{
  Q_OBJECT

public:
  pqMainWindow();
  ~pqMainWindow();

  /// Compares the contents of the window with the given reference image, returns true iff they "match" within some tolerance
  bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);

signals:
  void serverChanged();
  
private:
  void setServer(pqServer* Server);

  pqServer* CurrentServer;
  pqRefreshToolbar* RefreshToolbar;
  QToolBar* PropertyToolbar;
  pqMultiViewManager* MultiViewManager;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  pqPipelineData *Pipeline;
  QMenu* SourcesMenu;
  QMenu* FiltersMenu;
  pqObjectInspectorWidget *Inspector;
  QDockWidget *InspectorDock;
  pqPipelineListWidget *PipelineList;
  QDockWidget *PipelineDock;
  pqMultiViewFrame* ActiveView;

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
  void onUpdateSourcesFiltersMenu();
  void onHelpAbout();
  void onUpdateWindows();
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);
  
  void onRecordTest();
  void onRecordTest(const QStringList& Files);
  void onPlayTest();
  void onPlayTest(const QStringList& Files);
  
  void onPythonShell();
};

#endif // !_pqMainWindow_h

