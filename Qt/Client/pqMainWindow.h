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

class pqObjectInspector;
class pqObjectInspectorDelegate;
class pqRefreshToolbar;
class pqServer;
class pqSMAdaptor;

class QAction;
class QDockWidget;
class QToolBar;
class QTreeView;
class QVTKWidget;

/// Provides the main window for the ParaQ application
class pqMainWindow :
        public QMainWindow
{
  Q_OBJECT

public:
  pqMainWindow();
  ~pqMainWindow();

signals:
  void serverChanged();
  
private:
  void setServer(pqServer* Server);

  pqServer* CurrentServer;
  pqRefreshToolbar* RefreshToolbar;
  QToolBar* PropertyToolbar;
  QVTKWidget* Window;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  QMenu* SourcesMenu;
  pqObjectInspector *Inspector;
  pqObjectInspectorDelegate *InspectorDelegate;
  QDockWidget *InspectorDock;
  QTreeView *InspectorView;
  
private slots:
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
  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();
  void onUpdateSourcesMenu();
  void onHelpAbout();
  void onUpdateWindows();
  
  void onRecordTest();
  void onRecordTest(const QStringList& Files);
};

#endif // !_pqMainWindow_h

