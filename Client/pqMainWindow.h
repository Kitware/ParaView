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

class pqRefreshToolbar;
class pqServer;
class QAction;
class QToolBar;
class QVTKWidget;
class pqSMAdaptor;

class pqMainWindow :
        public QMainWindow
{
  typedef QMainWindow base;
  
  Q_OBJECT

public:
  pqMainWindow(QApplication& Application);
  ~pqMainWindow();
  
private:
  void setServer(pqServer* Server);

  pqServer* currentServer;
  pqRefreshToolbar* refresh_toolbar;
  QToolBar* property_toolbar;
  QVTKWidget* window;
  QAction* serverDisconnectAction;
  pqSMAdaptor *Adaptor;
  
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
  void onDebugOpenLocalFiles();
  void onDebugOpenLocalFiles(const QStringList& Files);
  void onDebugDumpQtHierarchy();
  void onTestsRun();
  
  void onDispatcherChanged();
  void onRedrawWindows();
};

#endif // !_pqMainWindow_h

