// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
