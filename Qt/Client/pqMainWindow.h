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
  void ServerChanged();
  
private:
  void SetServer(pqServer* Server);

  pqServer* CurrentServer;
  pqRefreshToolbar* RefreshToolbar;
  QToolBar* PropertyToolbar;
  QVTKWidget* Window;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  QMenu* SourcesMenu;
  pqObjectInspector *Inspector;
  QDockWidget *InspectorDock;
  QTreeView *InspectorView;
  
private slots:
  void OnFileNew();
  void OnFileNew(pqServer* Server);
  void OnFileOpen();
  void OnFileOpen(pqServer* Server);
  void OnFileOpen(const QStringList& Files);
  void OnFileOpenServerState();
  void OnFileOpenServerState(pqServer* Server);
  void OnFileOpenServerState(const QStringList& Files);
  void OnFileSaveServerState();
  void OnFileSaveServerState(const QStringList& Files);
  void OnServerConnect();
  void OnServerConnect(pqServer* Server);
  void OnServerDisconnect();
  void OnUpdateSourcesMenu();
  void OnHelpAbout();
  
  void OnUpdateWindows();
};

#endif // !_pqMainWindow_h

