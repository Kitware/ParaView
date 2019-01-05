/*=========================================================================

   Program: ParaView
   Module:    pqVRPNEventListener.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

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
    emit addConnectionInternal(conn);
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
    emit removeConnectionInternal(conn);
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
  emit listen();
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
  foreach (pqVRPNConnection* conn, this->Connections)
  {
    conn->listen();
  }
  // Immediately post another call to this method on the thread's event loop.
  // This way connections can be added and removed safely by serializing access
  // to the connection container via the event loop.
  QTimer::singleShot(0, this, SLOT(listen()));
}
