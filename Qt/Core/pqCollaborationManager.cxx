/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationManager.cxx

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
#include "pqCollaborationManager.h"

#include "pqApplicationCore.h"
#include "pqContextView.h"
#include "pqCoreUtilities.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkChart.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVMultiClientsInformation.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkWeakPointer.h"

#include <map>
#include <set>

// Qt includes.
#include <QAction>
#include <QEvent>
#include <QMap>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtDebug>

//***************************************************************************
//                           Internal class
//***************************************************************************
struct ChartBounds
{
  void SetRange(double* otherRange)
  {
    for (int i = 0; i < 8; i++)
    {
      this->Range[i] = otherRange[i];
    }
  }

  double Range[8];
};

class pqCollaborationManager::pqInternals
{
public:
  pqInternals(pqCollaborationManager* owner)
  {
    this->Owner = owner;
    this->BroadcastMouseLocation = false;
    this->CollaborativeTimer.setInterval(100);
    QObject::connect(&this->CollaborativeTimer, SIGNAL(timeout()), this->Owner,
      SLOT(sendMousePointerLocationToOtherClients()));
    QObject::connect(&this->CollaborativeTimer, SIGNAL(timeout()), this->Owner,
      SLOT(sendChartViewBoundsToOtherClients()));
    // we delay starting the timer till we have an active collaboration session.

    this->ProxyManager = vtkSMProxyManager::GetProxyManager();
    this->ProxyManagerObserverTag =
      this->ProxyManager->AddObserver(vtkSMProxyManager::ActiveSessionChanged, this,
        &pqCollaborationManager::pqInternals::activeServerChanged);
  }

  //-------------------------------------------------
  ~pqInternals()
  {
    if (this->ProxyManager)
    {
      this->ProxyManager->RemoveObserver(this->ProxyManagerObserverTag);
      this->ProxyManagerObserverTag = 0;
    }
  }
  //-------------------------------------------------
  void activeServerChanged(vtkObject*, unsigned long, void*)
  {
    this->ActiveCollaborationManager = NULL;
    this->AciveSession = NULL;
    this->CollaborativeTimer.stop();
    if (!this->ProxyManager)
    {
      return; // No more proxy manager
    }
    this->AciveSession = this->ProxyManager->GetActiveSession();
    if (this->AciveSession && this->AciveSession->IsMultiClients())
    {
      this->ActiveCollaborationManager = this->AciveSession->GetCollaborationManager();
      this->ActiveCollaborationManager->UpdateUserInformations();
      this->LastMousePointerPosition.set_client_id(this->ActiveCollaborationManager->GetUserId());
      this->CollaborativeTimer.start();
    }
    this->Owner->updateEnabledState();
  }

  //-------------------------------------------------
  void registerServer(pqServer* server)
  {
    if (server == NULL || !server->session()->GetCollaborationManager())
    {
      return;
    }
    QObject::connect(server, SIGNAL(sentFromOtherClient(pqServer*, vtkSMMessage*)), this->Owner,
      SLOT(onClientMessage(pqServer*, vtkSMMessage*)));
    QObject::connect(
      server, SIGNAL(triggeredMasterUser(int)), this->Owner, SIGNAL(triggeredMasterUser(int)));
    QObject::connect(
      server, SIGNAL(triggeredUserListChanged()), this->Owner, SIGNAL(triggeredUserListChanged()));
    QObject::connect(server, SIGNAL(triggeredUserName(int, QString&)), this->Owner,
      SIGNAL(triggeredUserName(int, QString&)));
    QObject::connect(
      server, SIGNAL(triggerFollowCamera(int)), this->Owner, SIGNAL(triggerFollowCamera(int)));

    this->VTKConnect->Connect(server->session()->GetCollaborationManager(),
      vtkSMCollaborationManager::CameraChanged, pqApplicationCore::instance(), SLOT(render()));
  }
  //-------------------------------------------------
  void unRegisterServer(pqServer* server)
  {
    if (server == NULL || !server->session()->GetCollaborationManager())
    {
      return;
    }
    QObject::disconnect(server, SIGNAL(sentFromOtherClient(pqServer*, vtkSMMessage*)), this->Owner,
      SLOT(onClientMessage(pqServer*, vtkSMMessage*)));
    QObject::disconnect(
      server, SIGNAL(triggeredMasterUser(int)), this->Owner, SIGNAL(triggeredMasterUser(int)));
    QObject::disconnect(
      server, SIGNAL(triggeredUserListChanged()), this->Owner, SIGNAL(triggeredUserListChanged()));
    QObject::disconnect(server, SIGNAL(triggeredUserName(int, QString&)), this->Owner,
      SIGNAL(triggeredUserName(int, QString&)));
    QObject::disconnect(
      server, SIGNAL(triggerFollowCamera(int)), this->Owner, SIGNAL(triggerFollowCamera(int)));

    this->VTKConnect->Disconnect(server->session()->GetCollaborationManager(),
      vtkSMCollaborationManager::CameraChanged, pqApplicationCore::instance(), SLOT(render()));
  }
  //-------------------------------------------------
  int GetClientId(int idx)
  {
    if (this->ActiveCollaborationManager)
    {
      return this->ActiveCollaborationManager->GetUserId(idx);
    }
    return -1;
  }

public:
  QMap<int, QString> UserNameMap;
  vtkWeakPointer<vtkSMSession> AciveSession;
  vtkWeakPointer<vtkSMCollaborationManager> ActiveCollaborationManager;
  vtkWeakPointer<vtkSMProxyManager> ProxyManager;
  vtkSMMessage LastMousePointerPosition;
  bool MousePointerLocationUpdated;
  bool BroadcastMouseLocation;
  std::map<vtkTypeUInt32, ChartBounds> ContextViewBoundsToShare;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

protected:
  QPointer<pqCollaborationManager> Owner;
  QTimer CollaborativeTimer;
  unsigned long ProxyManagerObserverTag;
};
//***************************************************************************/
pqCollaborationManager::pqCollaborationManager(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals(this);

  // Chat management + User list panel
  QObject::connect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
    SLOT(onChatMessage(pqServer*, int, QString&)));

  QObject::connect(this, SIGNAL(triggeredMasterUser(int)), this, SLOT(updateEnabledState()));
}

//-----------------------------------------------------------------------------
pqCollaborationManager::~pqCollaborationManager()
{
  // Chat management + User list panel
  QObject::disconnect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
    SLOT(onChatMessage(pqServer*, int, QString&)));

  delete this->Internals;
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onClientMessage(pqServer* server, vtkSMMessage* msg)
{
  if (msg->HasExtension(QtEvent::type))
  {
    int userId = 0;
    QString userName;
    QString chatMsg;
    switch (msg->GetExtension(QtEvent::type))
    {
      case QtEvent::CHAT:
        userId = msg->GetExtension(ChatMessage::author);
        userName = server->session()->GetCollaborationManager()->GetUserLabel(userId);
        chatMsg = msg->GetExtension(ChatMessage::txt).c_str();
        Q_EMIT triggerChatMessage(server, userId, chatMsg);
        break;
      case QtEvent::OTHER:
        // Custom handling
        break;
    }
  }
  else if (msg->HasExtension(MousePointer::view) &&
    (this->Internals->AciveSession ==
             server->session()) && // Make sure we share the same active server
    (msg->GetExtension(MousePointer::forceShow) ||
             (static_cast<int>(msg->client_id()) ==
               this->activeCollaborationManager()->GetFollowedUser())))
  {
    this->showMousePointer(msg->GetExtension(MousePointer::view),
      msg->GetExtension(MousePointer::x), msg->GetExtension(MousePointer::y),
      msg->GetExtension(MousePointer::ratioType));
  }
  else
  {
    Q_EMIT triggerStateClientOnlyMessage(server, msg);
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::onChatMessage(pqServer* server, int userId, QString& msgContent)
{
  // Broadcast to others only if its our message
  if (this->activeCollaborationManager() &&
    userId == this->activeCollaborationManager()->GetUserId())
  {
    vtkSMMessage chatMsg;
    chatMsg.SetExtension(QtEvent::type, QtEvent::CHAT);
    chatMsg.SetExtension(ChatMessage::author, this->activeCollaborationManager()->GetUserId());
    chatMsg.SetExtension(ChatMessage::txt, msgContent.toStdString());

    server->sendToOtherClients(&chatMsg);
  }
  else if (!this->activeCollaborationManager())
  {
    qDebug() << "The active server is not a Collaborative one.";
  }
}
//-----------------------------------------------------------------------------
vtkSMCollaborationManager* pqCollaborationManager::activeCollaborationManager()
{
  return this->Internals->ActiveCollaborationManager;
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::onServerAdded(pqServer* s)
{
  this->Internals->registerServer(s);
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::onServerRemoved(pqServer* s)
{
  this->Internals->unRegisterServer(s);
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::updateMousePointerLocation(QMouseEvent* e)
{
  pqQVTKWidget* widget = qobject_cast<pqQVTKWidget*>(QObject::sender());
  if (widget && this->activeCollaborationManager())
  {
    bool isChartView = (vtkSMContextViewProxy::SafeDownCast(
                          this->activeCollaborationManager()->GetSession()->GetRemoteObject(
                            widget->getProxyId())) != NULL);

    double w2 = widget->width() / 2;
    double h2 = widget->height() / 2;
    double px = (e->x() - w2) / (isChartView ? w2 : h2);
    double py = (e->y() - h2) / h2;

    this->Internals->LastMousePointerPosition.SetExtension(
      MousePointer::view, widget->getProxyId());
    this->Internals->LastMousePointerPosition.SetExtension(MousePointer::x, px);
    this->Internals->LastMousePointerPosition.SetExtension(MousePointer::y, py);
    this->Internals->LastMousePointerPosition.SetExtension(
      MousePointer::ratioType, isChartView ? MousePointer::BOTH : MousePointer::HEIGHT);
    this->Internals->MousePointerLocationUpdated = true;
  }
  else if (this->activeCollaborationManager())
  {
    qCritical("Invalid cast");
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::sendMousePointerLocationToOtherClients()
{
  if (this->Internals->BroadcastMouseLocation && this->Internals->MousePointerLocationUpdated &&
    this->activeCollaborationManager())
  {
    this->activeCollaborationManager()->SendToOtherClients(
      &this->Internals->LastMousePointerPosition);
    this->Internals->MousePointerLocationUpdated = false;
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::showMousePointer(
  vtkTypeUInt32 viewId, double x, double y, int useBothHeightWidth)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* view = smmodel->findItem<pqView*>(viewId);
  pqQVTKWidget* widget = NULL;
  if (view && (widget = qobject_cast<pqQVTKWidget*>(view->widget())))
  {
    double xRatioBase = 1;
    double yRatioBase = 1;
    double w2 = widget->width() / 2;
    double h2 = widget->height() / 2;
    switch (useBothHeightWidth)
    {
      case 0: // both
        xRatioBase = w2;
        yRatioBase = h2;
        break;
      case 1: // height
        xRatioBase = h2;
        yRatioBase = h2;
        break;
      case 2: // width
        xRatioBase = w2;
        yRatioBase = w2;
        break;
    }

    int px = static_cast<int>(xRatioBase * x + w2);
    int py = static_cast<int>(yRatioBase * y + h2);
    widget->paintMousePointer(px, py);
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::updateEnabledState()
{
  bool enabled =
    (this->activeCollaborationManager() ? this->activeCollaborationManager()->IsMaster() : true);
  QWidget* mainWidget = pqCoreUtilities::mainWidget();
  foreach (QWidget* wdg, mainWidget->findChildren<QWidget*>())
  {
    QVariant val = wdg->property("PV_MUST_BE_MASTER");
    if (val.isValid() && val.toBool())
    {
      wdg->setEnabled(enabled);
    }
    val = wdg->property("PV_MUST_BE_MASTER_TO_SHOW");
    if (val.isValid() && val.toBool())
    {
      wdg->setVisible(enabled);
    }
  }
  foreach (QAction* actn, mainWidget->findChildren<QAction*>())
  {
    // some actions are hidden, if the process is not a master.
    QVariant val = actn->property("PV_MUST_BE_MASTER_TO_SHOW");
    if (val.isValid() && val.toBool())
    {
      actn->setVisible(enabled);
    }
    // some other actions are merely 'blocked", if the process is not a master.
    // We don't use the enable/disable mechanism for actions, since most actions
    // are have their enabled state updated by logic that will need to be made
    // "collaboration aware" if we go that route. Instead, we install an event
    // filter that eats away clicks, instead.

    // Currently I cannot figure out how to do this. Event filters don't work on
    // Actions. There's no means of disabling action-activations besides marking
    // them disabled or hidden, it seems. block-signals seems to be the
    // crappiest way out of this. The problem is used has no indication with
    // block-signals that the action is not allowed in collaboration-mode. So
    // we'll stay away from it.
    val = actn->property("PV_MUST_BE_MASTER");
    if (val.isValid() && val.toBool())
    {
      actn->blockSignals(!enabled);
    }
  }

  Q_EMIT triggeredMasterChanged(enabled);
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::attachMouseListenerTo3DViews()
{
  // I am deliberately commenting this out. Collaboration is slated for
  // deprecation. Since we removed the unnecessary `mouseEvent`, it no longer
  // possible to support this. Rather than add more code to support a deprecated
  // use-case, we just drop it for now.

  // QWidget* mainWidget = pqCoreUtilities::mainWidget();
  // foreach (pqQVTKWidget* widget, mainWidget->findChildren<pqQVTKWidget*>())
  //{
  //  QObject::connect(widget, SIGNAL(mouseEvent(QMouseEvent*)), this,
  //    SLOT(updateMousePointerLocation(QMouseEvent*)), Qt::UniqueConnection);
  //}
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::enableMousePointerSharing(bool enable)
{
  this->Internals->BroadcastMouseLocation = enable;
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::disableFurtherConnections(bool disable)
{
  if (this->Internals->ActiveCollaborationManager)
  {
    this->Internals->ActiveCollaborationManager->DisableFurtherConnections(disable);
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::setConnectID(int connectID)
{
  if (this->Internals->ActiveCollaborationManager)
  {
    this->Internals->ActiveCollaborationManager->SetConnectID(connectID);
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::onChartViewChange(vtkTypeUInt32 gid, double* bounds)
{
  pqContextView* chartView = qobject_cast<pqContextView*>(QObject::sender());
  if (chartView && activeCollaborationManager() &&
    activeCollaborationManager()->GetSession() == chartView->getServer()->session())
  {
    this->Internals->ContextViewBoundsToShare[gid].SetRange(bounds);
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::sendChartViewBoundsToOtherClients()
{
  // FIXME:UDA
  // if(this->Internals->ContextViewBoundsToShare.size() > 0)
  //   {
  //   std::map<vtkTypeUInt32, ChartBounds>::iterator iter;
  //   iter = this->Internals->ContextViewBoundsToShare.begin();
  //   while(iter != this->Internals->ContextViewBoundsToShare.end())
  //     {
  //     vtkSMMessage msg;
  //     msg.SetExtension(QtEvent::type, QtEvent::CHART_BOUNDS);
  //     msg.SetExtension(ChartViewBounds::view, iter->first);
  //     for(int i=0;i<8;i++)
  //       {
  //       msg.AddExtension(ChartViewBounds::range, iter->second.Range[i]);
  //       }

  //     this->activeCollaborationManager()->SendToOtherClients(&msg);

  //     // Move forward
  //     iter++;
  //     }
  //   // Clean up stack
  //   this->Internals->ContextViewBoundsToShare.clear();
  //   }
}
