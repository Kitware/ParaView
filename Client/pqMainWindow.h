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

class pqServer;
class QAction;
class QVTKWidget;

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
  QVTKWidget* window;
  QAction* serverDisconnectAction;
  
private slots:
  void onFileNew();
  void onFileNew(pqServer* Server);
  void onFileOpen();
  void onFileOpen(pqServer* Server);
  void onFileOpen(const QStringList& Files);
  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();
  void onDebugOpenLocalFiles();
  void onDebugOpenLocalFiles(const QStringList& Files);
  void onDebugQtHierarchy();
  void onTestsRun();
};

#endif // !_pqMainWindow_h

