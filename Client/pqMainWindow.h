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
  Q_OBJECT

public:
  pqMainWindow(QApplication& Application);
  ~pqMainWindow();
  
private:
  void setServer(pqServer* Server);

  pqServer* currentServer;
  QVTKWidget* window;  // just a random place to put this for now
  
  QAction* fileOpenAction;
  QAction* serverConnectAction;
  QAction* serverDisconnectAction;
  
private slots:
  void onFileOpen();
  void onFileOpen(const QString& File);
  void onServerConnect();
  void onServerDisconnect();
  void onDebugHierarchy();
  void onTestsRun();
};

#endif // !_pqMainWindow_h

