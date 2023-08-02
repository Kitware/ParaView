// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqCollaborationPanel(QWidget* parent = nullptr);
  ~pqCollaborationPanel() override;

Q_SIGNALS:
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

public Q_SLOTS:
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

protected Q_SLOTS:
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
   * Called when the user change its name (double click in the table on its
   * name)
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

protected: // NOLINT(readability-redundant-access-specifiers)
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
