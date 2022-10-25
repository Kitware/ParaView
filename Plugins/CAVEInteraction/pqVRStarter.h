/*=========================================================================

   Program: ParaView
   Module:  pqVRStarter.h

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
#ifndef pqVRStarter_h
#define pqVRStarter_h
#include <QObject>

class QTimer;
class vtkVRQueue;
class pqVRQueueHandler;
class vtkPVXMLElement;
class vtkSMProxyLocator;

/// pqVRStarter creates and establishes the framework for VR device. There are
/// three primary objects that make up the framework, vtkVRConnectionManager,
/// vtkVRQueue and VRQueueHandler. vtkVRConnectionManager and the
/// vtkVRQueueHandler are threads one acting as a source and other as the
/// destination for coming in from various VR connection servers (VRPN and
/// VRUI). vtkVRQueue is a asynchronous queue established between the
/// VRConnectionManager and the vtkVRQueueHandler.
class pqVRStarter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqVRStarter(QObject* _parent = 0);
  ~pqVRStarter();

  /// Creates and initiates the vtkVRConnectionManager thread the vtkVRQueue and
  /// vtkVRQueueHandler thread.
  void onStartup();

  /// Stops the vtkVRConnectioManager and vtkVRQueueHandler threads. Also
  /// deletes the corresponding objects including vtkVRQueue.
  void onShutdown();

private:
  Q_DISABLE_COPY(pqVRStarter)

  class pqInternals;
  pqInternals* Internals;
  bool IsShutdown;
};

#endif // pqVRStarter_h
