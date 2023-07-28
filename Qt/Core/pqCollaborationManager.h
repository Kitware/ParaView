// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCollaborationManager_h
#define pqCollaborationManager_h

#include "pqCoreModule.h"
#include "vtkSMMessageMinimal.h"
#include <QObject>

class vtkObject;
class pqServer;
class pqView;
class pqPipelineSource;
class vtkSMCollaborationManager;
class QMouseEvent;

/**
 * pqCollaborationManager is a QObject that aims to handle the collaboration
 * for the Qt layer. This class is used to synchronize the ActiveObject across
 * client instances as well as managing the rendering request when data has
 * been changed by other clients.
 * This class is responsible to synchronize
 *    - rendering request
 *    - pqProxy internal state
 *    - master/slave (enable/disable edition control in UI)
 *    - selected active source
 */

class PQCORE_EXPORT pqCollaborationManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCollaborationManager(QObject* parent);
  ~pqCollaborationManager() override;

  /**
   * Return the vtkSMCollaborationManager
   */
  vtkSMCollaborationManager* activeCollaborationManager();

Q_SIGNALS:
  /**
   * This will be triggered by the remote clients to update any interesting
   * components. This should be triggered by local client to broadcast to
   * the other clients
   */
  void triggerChatMessage(pqServer* server, int userId, QString& msgContent);

  /**
   * This will forward client_only message to anyone that may interessted when
   * not managed locally
   */
  void triggerStateClientOnlyMessage(pqServer* origin, vtkSMMessage* msg);

  /**
   * Signal triggered when user information get updated
   * regardless the active one
   * A nice thing TODO could be to just forward from the pqServer but ONLY IF
   * (activeServer == sender)
   */
  void triggeredMasterUser(int);
  void triggeredMasterChanged(bool);
  void triggeredUserName(int, QString&);
  void triggeredUserListChanged();

  /**
   * This will be triggered when a remote master client has requested the other
   * users to follow a given camera
   */
  void triggerFollowCamera(int);

public Q_SLOTS:
  /**
   * Slot used to keep track of all possible vtkSMCollaborationManagers
   * They are used in pqCollaborationBehavior to listen to the
   * ServerManagerModel... (preServerAdded/aboutToRemoveServer)
   */
  void onServerAdded(pqServer*);
  void onServerRemoved(pqServer*);

  /**
   * This will be triggered by the triggerChatMessage() signal and will
   * broadcast to other client a chat message
   */
  void onChatMessage(pqServer* server, int userId, QString& msgContent);

  /**
   * updates the enabled-state for application wide widgets and actions based
   * whether the application is a master or not.
   * Widget/Actions need to set a dynamic property named PV_MUST_BE_MASTER or
   * PV_MUST_BE_MASTER_TO_SHOW. Only the state for widgets/actions with these
   * any of properties will be updated by this method.
   */
  void updateEnabledState();

  /**
   * Method called localy when user want to broadcast its pointer to
   * other users.
   */
  void updateMousePointerLocation(QMouseEvent* e);

  /**
   * Method triggered by the internal collaboration Timer.
   * This timer prevent a network overload by only sending the latest
   * mouse location to the other clients every 100 ms
   */
  void sendMousePointerLocationToOtherClients();

  /**
   * Method triggered by the internal collaboration Timer.
   * This timer prevent a network overload by only sending the latest
   * modified view location to the other clients every 100 ms
   */
  void sendChartViewBoundsToOtherClients();

  /**
   * Attach a mouse listener if its a 3D view so we can share that
   * information with other clients
   */
  void attachMouseListenerTo3DViews();

  /**
   * Enable/disable local mouse pointer location
   */
  void enableMousePointerSharing(bool);

  /**
   * Enable/disable further connections to the server.
   */
  void disableFurtherConnections(bool disable);

  /**
   * Set the connect-id.
   */
  void setConnectID(int connectID);

private Q_SLOTS:
  /**
   * Called when a message has been sent by another client
   * This method will trigger signals that will be used by other Qt classes
   * to synchronize their state.
   */
  void onClientMessage(pqServer* server, vtkSMMessage* msg);

  /**
   * Called when a chart view has changed is view bounds
   */
  void onChartViewChange(vtkTypeUInt32 gid, double* bounds);

  /**
   * Show another mouse pointer as overlay to a 3D view.
   * x et y are normalized based on height/2 where 0 is the center of the
   * view.
   * ratioToUse = [both:0, height:1, width: 2]
   */
  void showMousePointer(vtkTypeUInt32 viewId, double x, double y, int ratioToUse);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCollaborationManager)

  class pqInternals;
  pqInternals* Internals;
};

#endif // !pqCollaborationManager_h
