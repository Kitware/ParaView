
//--------------------------------------------------------------------------
//
// This file is part of the Vistrails ParaView Plugin.
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of
// this file.  Please review the following to ensure GNU General Public
// Licensing requirements will be met:
// http://www.opensource.org/licenses/gpl-2.0.php
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Copyright (C) 2009 VisTrails, Inc. All rights reserved.
//
//--------------------------------------------------------------------------

#ifndef PLUGIN_MAIN_H
#define PLUGIN_MAIN_H

#include <QActionGroup>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QThread>
#include <QProcess>
#include <QList>
#include <QHash>
#include <QQueue>
#include <QTcpSocket>

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkUndoSet.h"



class SocketHelper {
public:
  SocketHelper(QTcpSocket *sock);

  bool readInt(int &i);
  bool readString(QString &s);
  bool readData(int size, QByteArray &data);

  bool writeInt(int i);
  bool writeString(QString s);

  bool waitForBytesWritten(int timeout);
  void moveToThread(QThread *thread);
  void close();

private:
  QTcpSocket *sock;
  QByteArray buffer;
};




class PluginMain : public QThread
{
   Q_OBJECT

public:
  PluginMain();

  void Startup();
  void Shutdown();

  void run();

public slots:
  void handleStackChanged(bool canUndo, QString undoLabel,
    bool canRedo, QString redoLabel);

private:

  // The VisTrails process that we spawn
  QProcess visTrails;

  // Connection that we can send commands to VisTrails through.
  SocketHelper *vtSender;

  // ParaView's undo stack
  // FIXME - keep this as a member or not?
  pqUndoStack *undoStack;

  // We need to keep track of the active server - our tracking of 
  // it is taken from pqMainWindowCore
  pqServer* activeServer;

  // We want to avoid processing stack changed signals when we're 
  // fiddling with the undo stack.
  volatile bool ignoreStackSignal;

  // After the stateLoaded signal gets sent, we expect to see a server
  // resource change that we can get the filename of the state from.
  bool stateLoading;

  // You'd think that we could just use QCoreApplication::closingDown(),
  // but apparently that doesn't work like you'd expect.  Instead, we
  // setup a slot to catch the aboutToQuit() signal, and set this flag.
  volatile bool quitting;

  // If we got a message that VisTrails is shutting down, we don't send
  // the shutdown message when ParaView is shutting down
  volatile bool vistrailsShutdown;

  // We keep track of the VisTrails pipline ids as the user changes
  // it in ParaView.  This is a list of version numbers that goes from
  // the initial version (0) to the farthest one we can redo to.
  // The current version is versionStack[versionStackIndex].
  QList<int> versionStack;
  int versionStackIndex;

  // This helper looks at the series of version transitions that
  // VisTrails wants us to perform.  This involves first going "up" the
  // version tree, undoing some operations.  Then we go back down
  // do'ing (or maybe redo'ing if we're in the same branch) until
  // we get to the version we want.  This function retreives the
  // xml from VisTrails for the version transitions that we can't
  // perform with simple undo or redos.
  void fetchVersionDeltas(const QList<int> &versions, int commonIndex,
    int first, QList<QString> &labels, QList<QString> &deltas);

  // We need to move the vtSender socket to the main thread since
  // qt only really allows using a socket from the thread it was created
  // in. Ideally, we could just pass this to run() as a parameter.
  QThread *mainThread;

  // The listener thread has to trigger anything to do through the 
  // signal/slot mechanism, otherwise there are strange threading
  // issues apparently.
signals:
  void modifyStackSignal(QList<int> versions, int commonIndex);
  void resetSignal(int maxId);

public slots:
  void modifyStackSlot(QList<int> versions, int commonIndex);
  void resetSlot(int maxId);
  void aboutToQuit();

  void stateLoaded();
  void serverResourcesChanged();

  void serverAdded(pqServer *server);
  void serverRemoved(pqServer *server);

};


#endif
