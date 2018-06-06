/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationPanel.h

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

========================================================================*/
#ifndef pqCollaborationPanel_h
#define pqCollaborationPanel_h

#include "pqComponentsModule.h"
#include "vtkSMMessageMinimal.h"
#include <QWidget>

class pqServer;
class pqView;
class pqCollaborationManager;
class QTableWidgetItem;
class vtkSMCollaborationManager;

/**
* pqCollaborationPanel is a properties page for the collaborative session. It
* allows the user to change its name and manage leadership of the session.
*/
class PQCOMPONENTS_EXPORT pqCollaborationPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqCollaborationPanel(QWidget* parent = 0);
  ~pqCollaborationPanel() override;

signals:
  /**
  * Emitting this signal will result by adding the message into the UI and
  * if the user is the local one, the message will be broadcasted to the
  * other clients.
  */
  void triggerChatMessage(pqServer* server, int userId, QString& msgContent);

  /**
  * Emitting this signal when user has allowed its mouse position to be
  * shared and send to the other clients
  */
  void shareLocalMousePointer(bool);

  /**
   * This signal is triggered when user has allowed/disallowed
   * further connections to the server.
   */
  void disableFurtherConnections(bool);

  /**
  * This get triggered internally when it's not a good time to update the camera
  * so the request get pushed to QueuedConnection
  */
  void delayUpdateCamera(vtkSMMessage* msg);

  /**
   * This signal is triggered when user changes the connect-id.
   */
  void connectIDChanged(int);

public slots:
  /**
  * Called by pqCollaborationManager when a message is received
  */
  void writeChatMessage(pqServer* server, int userId, QString& txt);
  /**
  * Called by pqCollaborationManager when a user name update occurs
  * (this invalidate the table model)
  */
  void onUserUpdate();
  /**
  * Called when a new master has been promoted
  */
  void onNewMaster(int);

protected slots:
  /**
  * Called when user hit enter in the input line of chat message
  */
  void onUserMessage();

  /**
  * Called when pqView are added/removed so we can listen user interaction
  */
  void connectViewLocalSlots(pqView*);
  void disconnectViewLocalSlots(pqView*);
  void stopFollowingCamera();

  /**
  * Called when the user change its name
  * (double click in the table on his name)
  */
  void itemChanged(QTableWidgetItem* item);

  /**
  * Called when the user double click on any cell
  */
  void cellDoubleClicked(int, int);

  /**
  * Called when to follow a given user camera
  */
  void followUserCamera(int userId);

  void onServerChanged();

  /**
   * Called when the user changes the connect-id. Emit signal with new value.
   */
  void onConnectIDChanged();

protected:
  /**
  * Promote a new master
  */
  void promoteToMaster(int masterId);

  Q_DISABLE_COPY(pqCollaborationPanel)

  pqCollaborationManager* getCollaborationManager();
  vtkSMCollaborationManager* getSMCollaborationManager();

  class pqInternal;
  pqInternal* Internal;
};

#endif
