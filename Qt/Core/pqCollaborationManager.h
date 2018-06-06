/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationManager.h

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
#ifndef _pqCollaborationManager_h
#define _pqCollaborationManager_h

#include "pqCoreModule.h"
#include "vtkSMMessageMinimal.h"
#include <QObject>

class vtkObject;
class pqServer;
class pqView;
class pqPipelineSource;
class vtkSMCollaborationManager;
class QSignalMapper;
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

signals:
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

public slots:
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

private slots:
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

private:
  Q_DISABLE_COPY(pqCollaborationManager)

  class pqInternals;
  pqInternals* Internals;
};

#endif // !_pqCollaborationManager_h
