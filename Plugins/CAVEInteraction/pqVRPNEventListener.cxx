// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqVRPNEventListener.h"

#include "pqVRPNConnection.h"

#include <QtCore/QDebug> // For qWarning
#include <QtCore/QList>
#include <QtCore/QTimer>

//------------------------------------------------------------------------------
pqVRPNEventListener::pqVRPNEventListener(QObject* _parent)
  : Superclass(_parent)
  , Bridge(new pqVRPNThreadBridge)
{
  this->Bridge->moveToThread(&this->Thread);
  connect(this, SIGNAL(addConnectionInternal(pqVRPNConnection*)), this->Bridge,
    SLOT(addConnection(pqVRPNConnection*)));
  connect(this, SIGNAL(removeConnectionInternal(pqVRPNConnection*)), this->Bridge,
    SLOT(removeConnection(pqVRPNConnection*)));
  connect(this, SIGNAL(listen()), this->Bridge, SLOT(listen()));
}

//------------------------------------------------------------------------------
pqVRPNEventListener::~pqVRPNEventListener()
{
  if (this->isRunning())
  {
    this->stop();
  }
  delete Bridge;
}

//------------------------------------------------------------------------------
void pqVRPNEventListener::addConnection(pqVRPNConnection* conn)
{
  this->connect(conn, SIGNAL(destroyed()), this, SLOT(removeSenderConnection()));

  if (this->isRunning())
  {
    // Need to synchronize and use signals/slots if running.
    this->Bridge->SyncMutex.lock();
    Q_EMIT addConnectionInternal(conn);
    this->Bridge->SyncCondition.wait(&this->Bridge->SyncMutex);
    this->Bridge->SyncMutex.unlock();
  }
  else
  {
    // Otherwise just call the slot directly.
    this->Bridge->addConnection(conn);
    this->start();
  }
}

//------------------------------------------------------------------------------
void pqVRPNThreadBridge::addConnection(pqVRPNConnection* conn)
{
  this->SyncMutex.lock();
  if (!this->Connections.contains(conn))
  {
    this->Connections.append(conn);
  }
  this->SyncMutex.unlock();
  this->SyncCondition.wakeAll();
}

//------------------------------------------------------------------------------
void pqVRPNEventListener::removeConnection(pqVRPNConnection* conn)
{
  this->disconnect(conn);
  if (this->isRunning())
  {
    // Need to synchronize and use signals/slots if running.
    this->Bridge->SyncMutex.lock();
    Q_EMIT removeConnectionInternal(conn);
    this->Bridge->SyncCondition.wait(&this->Bridge->SyncMutex);
    bool needsStop = this->Bridge->Connections.isEmpty();
    this->Bridge->SyncMutex.unlock();
    if (needsStop)
    {
      this->stop();
    }
  }
  else
  {
    // Otherwise just call the slot directly.
    this->Bridge->removeConnection(conn);
  }
}

//------------------------------------------------------------------------------
void pqVRPNThreadBridge::removeConnection(pqVRPNConnection* conn)
{
  this->SyncMutex.lock();
  this->Connections.removeOne(conn);
  this->SyncMutex.unlock();
  this->SyncCondition.wakeAll();
}

//------------------------------------------------------------------------------
void pqVRPNEventListener::removeSenderConnection()
{
  if (pqVRPNConnection* conn = qobject_cast<pqVRPNConnection*>(this->sender()))
  {
    this->removeConnection(conn);
  }
}

//------------------------------------------------------------------------------
void pqVRPNEventListener::start()
{
  if (this->isRunning())
  {
    qWarning("pqVRPNEventListener::start() called while already running!");
    return;
  }
  this->Thread.start();
  Q_EMIT listen();
}

//------------------------------------------------------------------------------
void pqVRPNEventListener::stop()
{
  if (!this->isRunning())
  {
    qWarning("pqVRPNEventListener::stop() called while not running.");
  }
  this->Thread.exit(0);
}

//------------------------------------------------------------------------------
void pqVRPNThreadBridge::listen()
{
  Q_FOREACH (pqVRPNConnection* conn, this->Connections)
  {
    conn->listen();
  }
  // Immediately post another call to this method on the thread's event loop.
  // This way connections can be added and removed safely by serializing access
  // to the connection container via the event loop.
  QTimer::singleShot(0, this, SLOT(listen()));
}
