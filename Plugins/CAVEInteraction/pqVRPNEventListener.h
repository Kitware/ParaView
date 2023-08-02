// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqVRPNEventListener_h
#define pqVRPNEventListener_h

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

class pqVRPNConnection;
class pqVRPNThreadBridge; // Defined below
class pqVRPNEventListener : public QObject
{
  Q_OBJECT
public:
  typedef QObject Superclass;
  explicit pqVRPNEventListener(QObject* _parent = nullptr);
  ~pqVRPNEventListener();

  // Description:
  // Returns true if the listener has started.
  bool isRunning() const { return this->Thread.isRunning(); }

public Q_SLOTS:
  // Description:
  // Register a connection with the listener. The listener will automatically
  // start listening for events when the first connection is registered. The
  // connection will be removed automatically when it is deleted, or when
  // removeConnection is called. The listener will stop listening once all
  // connection are removed.
  void addConnection(pqVRPNConnection* conn);

  // Description:
  // Remove a connection from the listener.
  void removeConnection(pqVRPNConnection* conn);

  // Description:
  // Remove the pqVRPNConnection that is the Qt signal/slot sender which
  // triggered this call.
  void removeSenderConnection();

Q_SIGNALS:

  // Description:
  // Internal use only.
  void addConnectionInternal(pqVRPNConnection* conn);
  void removeConnectionInternal(pqVRPNConnection* conn);
  void listen();

private:
  // Description:
  // Start the listener.
  void start();

  // Description:
  // Stop the listener.
  void stop();

  pqVRPNThreadBridge* Bridge;
  QThread Thread;
};

class pqVRPNThreadBridge : public QObject
{
  Q_OBJECT
public:
  QMutex SyncMutex;
  QWaitCondition SyncCondition;
  QList<pqVRPNConnection*> Connections;

public Q_SLOTS:
  void addConnection(pqVRPNConnection* conn);
  void removeConnection(pqVRPNConnection* conn);

protected Q_SLOTS:
  void listen();
};

#endif
