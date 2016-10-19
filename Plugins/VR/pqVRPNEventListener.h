/*=========================================================================

   Program: ParaView
   Module:    pqVRPNEventListener.h

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
  explicit pqVRPNEventListener(QObject* _parent = NULL);
  ~pqVRPNEventListener();

  // Description:
  // Returns true if the listener has started.
  bool isRunning() const { return this->Thread.isRunning(); }

public slots:
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

signals:

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

public slots:
  void addConnection(pqVRPNConnection* conn);
  void removeConnection(pqVRPNConnection* conn);

protected slots:
  void listen();
};

#endif
