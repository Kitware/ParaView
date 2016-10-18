/*=========================================================================

   Program: ParaView
   Module:    pqRemoteControl.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef _pqRemoteControl_h
#define _pqRemoteControl_h

#include <QDockWidget>

class QHostInfo;
class pqRenderView;
class vtkSMViewProxy;
class vtkRenderWindow;

/// This is a dock widget that implements the user interface for the
/// mobile remote control plugin.  It works in collaboration with the
/// pqRemoteControlThread to create a server that can talk to a client
/// running on a mobile device.  The client can receive the ParaView
/// render view scene, and the client can send camera state information
/// to the server.  This allows you to use a mobile device to view the
/// ParaView scene and control the ParaView camera.
class pqRemoteControl : public QDockWidget
{
  Q_OBJECT
public:
  pqRemoteControl(QWidget* p = 0, Qt::WindowFlags flags = 0);
  virtual ~pqRemoteControl();

protected slots:

  /// Called when the Start or Stop button is clicked by the user
  void onButtonClicked();

  /// Called when a hyperlink is clicked in the documentation label text.
  void onLinkClicked(const QString& link);

  /// A callback to receive host lookup information
  void onHostLookup(const QHostInfo& host);

  /// A timer callback to check the status of the server socket connection periodically.
  void checkForConnection();

  /// A callback to check for new camera state information, and update the render
  /// view's camera if there is new information.
  void updateCamera();

  /// A callback to export the scene.  This is called from the remote control
  /// thread using a threadsafe slot connection.  The export takes place on the
  /// main GUI thread, and the GUI will be blocked during the export.
  void onExportScene();

protected:
  /// Called when a client has established a socket connection.  This will
  /// begin the remote control thread.
  void onNewConnection();

  /// Called in order to open the server socket.  This starts a timer to
  /// call checkForConnection() periodically.
  void onStart();

  /// Called to stop the remote control thread and close the server socket.
  void onStop();

  /// Return the render view that is being controlled.
  pqRenderView* renderView();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
