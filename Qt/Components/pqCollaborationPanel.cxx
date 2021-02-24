/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationPanel.cxx

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
#include "pqCollaborationPanel.h"

// Qt Includes.
#include <QCheckBox>
#include <QColor>
#include <QDockWidget>
#include <QPointer>
#include <QScrollArea>
#include <QTextCursor>
#include <QTextDocument>
#include <QtDebug>

//// ParaView Includes.
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCollaborationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include "vtkCommand.h"
#include <map>
#include <sstream>
#include <string>
#include <vtkNew.h>

//*****************************************************************************
#include "ui_pqCollaborationPanel.h"
class pqCollaborationPanel::pqInternal : public Ui::pqCollaborationPanel
{
public:
  bool NeedToConnectToCollaborationManager;
  int CameraToFollowOfUserId;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  std::map<int, vtkSMMessage> LocalCameraStateCache;
  vtkSMCollaborationManager* LastSeenCollaborationManager;
};
//-----------------------------------------------------------------------------
pqCollaborationPanel::pqCollaborationPanel(QWidget* p)
  : Superclass(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->members->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  this->Internal->members->horizontalHeader()->setSectionResizeMode(
    1, QHeaderView::ResizeToContents);
  this->Internal->CameraToFollowOfUserId = -1;
  this->Internal->NeedToConnectToCollaborationManager = true;
  this->Internal->connectId->setMaximum(VTK_INT_MAX);
  this->Internal->disableFurtherConnections->setChecked(false);
  this->Internal->masterControl->setVisible(false);

  QObject::connect(this->Internal->message, SIGNAL(returnPressed()), this, SLOT(onUserMessage()));

  QObject::connect(this->Internal->members, SIGNAL(itemChanged(QTableWidgetItem*)), this,
    SLOT(itemChanged(QTableWidgetItem*)));

  QObject::connect(this->Internal->members, SIGNAL(cellDoubleClicked(int, int)), this,
    SLOT(cellDoubleClicked(int, int)));

  QObject::connect(this->Internal->shareMousePointer, SIGNAL(clicked(bool)), this,
    SIGNAL(shareLocalMousePointer(bool)));

  QObject::connect(this->Internal->disableFurtherConnections, SIGNAL(clicked(bool)), this,
    SIGNAL(disableFurtherConnections(bool)));

  QObject::connect(
    this->Internal->connectId, SIGNAL(editingFinished()), this, SLOT(onConnectIDChanged()));

  QObject::connect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
    SLOT(writeChatMessage(pqServer*, int, QString&)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)), this, SLOT(connectViewLocalSlots(pqView*)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(preViewRemoved(pqView*)), this, SLOT(disconnectViewLocalSlots(pqView*)));

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this, SLOT(onServerChanged()));
}

//-----------------------------------------------------------------------------
pqCollaborationPanel::~pqCollaborationPanel()
{
  QObject::disconnect(
    this->Internal->message, SIGNAL(returnPressed()), this, SLOT(onUserMessage()));

  QObject::disconnect(this->Internal->members, SIGNAL(itemChanged(QTableWidgetItem*)), this,
    SLOT(itemChanged(QTableWidgetItem*)));
  QObject::disconnect(this->Internal->members, SIGNAL(cellDoubleClicked(int, int)), this,
    SLOT(cellDoubleClicked(int, int)));

  QObject::disconnect(this->Internal->shareMousePointer, SIGNAL(clicked(bool)), this,
    SIGNAL(shareLocalMousePointer(bool)));

  QObject::disconnect(this->Internal->disableFurtherConnections, SIGNAL(clicked(bool)), this,
    SIGNAL(disableFurtherConnections(bool)));

  QObject::disconnect(
    this->Internal->connectId, SIGNAL(editingFinished()), this, SLOT(onConnectIDChanged()));

  QObject::disconnect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
    SLOT(writeChatMessage(pqServer*, int, QString&)));

  QObject::disconnect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this, SLOT(onServerChanged()));

  pqCollaborationManager* collab = this->getCollaborationManager();
  if (collab)
  {
    // If we disconnect, this means a new session has been created so all
    // objects that we observed have been deleted
    QObject::disconnect(collab, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
      SLOT(writeChatMessage(pqServer*, int, QString&)));
    QObject::disconnect(collab, SIGNAL(triggeredUserListChanged()), this, SLOT(onUserUpdate()));

    QObject::disconnect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), collab,
      SLOT(onChatMessage(pqServer*, int, QString&)));

    QObject::disconnect(
      this, SIGNAL(shareLocalMousePointer(bool)), collab, SLOT(enableMousePointerSharing(bool)));

    QObject::disconnect(
      this, SIGNAL(disableFurtherConnections(bool)), collab, SLOT(disableFurtherConnections(bool)));

    QObject::disconnect(this, SIGNAL(connectIDChanged(int)), collab, SLOT(setConnectID(int)));

    QObject::disconnect(collab, SIGNAL(triggeredMasterUser(int)), this, SLOT(onNewMaster(int)));

    QObject::disconnect(
      collab, SIGNAL(triggerFollowCamera(int)), this, SLOT(followUserCamera(int)));
  }

  delete this->Internal;
  this->Internal = nullptr;
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserMessage()
{
  if (this->Internal->message->text().trimmed().length() == 0)
  {
    return;
  }

  vtkSMCollaborationManager* collab = getSMCollaborationManager();
  if (collab)
  {
    pqServer* activeServer =
      pqApplicationCore::instance()->getServerManagerModel()->findServer(collab->GetSession());
    int userId = collab->GetUserId();
    QString msg = this->Internal->message->text();
    Q_EMIT triggerChatMessage(activeServer, userId, msg);
    this->Internal->message->clear();
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::writeChatMessage(pqServer* server, int userId, QString& txt)
{
  QString message =
    QString("<b>%1:</b> %2 <br/>\n\n")
      .arg(server->session()->GetCollaborationManager()->GetUserLabel(userId), txt.trimmed());

  this->Internal->content->textCursor().atEnd();
  this->Internal->content->insertHtml(message);
  this->Internal->content->textCursor().atEnd();
  this->Internal->content->textCursor().movePosition(QTextCursor::End);
  this->Internal->content->ensureCursorVisible();
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::itemChanged(QTableWidgetItem* item)
{
  if (item->column() == 0)
  {
    vtkSMCollaborationManager* collab = this->getSMCollaborationManager();
    if (collab)
    {
      int id = item->data(Qt::UserRole).toInt();
      if (collab->GetUserId() == id)
      {
        QString userName = item->text();
        if (userName != collab->GetUserLabel(id))
        {
          collab->SetUserLabel(id, userName.toLocal8Bit().data());
        }
      }
    }
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::cellDoubleClicked(int row, int col)
{
  int userId = this->Internal->members->item(row, 0)->data(Qt::UserRole).toInt();
  switch (col)
  {
    case 1: // Camera Link
      this->followUserCamera(userId);
      break;
    case 0: // Master user
      this->promoteToMaster(userId);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::followUserCamera(int userId)
{
  // Update collaboration manager to know if we follow a given user or not
  if (this->getSMCollaborationManager())
  {
    this->getSMCollaborationManager()->FollowUser(userId);
  }

  if (this->Internal->CameraToFollowOfUserId == userId ||
    this->getSMCollaborationManager() == nullptr)
  {
    return;
  }

  // Update user Camera to follow
  if (this->getSMCollaborationManager()->GetUserId() == userId)
  {
    this->Internal->CameraToFollowOfUserId = 0; // Looking at our local camera
  }
  else
  {
    this->Internal->CameraToFollowOfUserId = userId;
  }

  // Update the collaboration manager
  this->getSMCollaborationManager()->FollowUser(userId);

  // Update the UI
  int nbRows = this->Internal->members->rowCount();
  for (int i = 0; i < nbRows; i++)
  {
    if (userId == this->Internal->members->item(i, 0)->data(Qt::UserRole).toInt())
    {
      this->Internal->members->item(i, 1)->setIcon(QIcon(":/pqWidgets/Icons/pqEyeball.svg"));
    }
    else
    {
      this->Internal->members->item(i, 1)->setIcon(QIcon());
    }
  }

  // If we follow master lets selection model follow as well
  bool followMaster = (userId == this->getSMCollaborationManager()->GetMasterId());
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  for (vtkIdType idx = 0; idx < pxm->GetNumberOfSelectionModel(); idx++)
  {
    vtkSMProxySelectionModel* selectionModel = pxm->GetSelectionModelAt(idx);
    selectionModel->SetFollowingMaster(followMaster);
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onNewMaster(int masterId)
{
  // When a new master is elected, we just by default get our camera synch to his
  this->followUserCamera(masterId);

  // Update the UI
  int nbRows = this->Internal->members->rowCount();
  for (int i = 0; i < nbRows; i++)
  {
    if (masterId == this->Internal->members->item(i, 0)->data(Qt::UserRole).toInt())
    {
      this->Internal->members->item(i, 0)->setIcon(QIcon(":/pqWidgets/Icons/pqMousePick15.png"));
    }
    else
    {
      this->Internal->members->item(i, 0)->setIcon(QIcon());
    }
  }

  vtkSMCollaborationManager* collabManager = this->getSMCollaborationManager();
  if (collabManager != nullptr)
  {
    this->Internal->masterControl->setVisible(collabManager->IsMaster());
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::promoteToMaster(int masterId)
{
  if (this->getSMCollaborationManager())
  {
    vtkSMCollaborationManager* collabManager = this->getSMCollaborationManager();
    if (collabManager->GetMasterId() == collabManager->GetUserId())
    {
      // We tell everyone, who's the new master
      collabManager->PromoteToMaster(masterId);
    }
  }
}

//-----------------------------------------------------------------------------
pqCollaborationManager* pqCollaborationPanel::getCollaborationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  return qobject_cast<pqCollaborationManager*>(core->manager("COLLABORATION_MANAGER"));
}
//-----------------------------------------------------------------------------
vtkSMCollaborationManager* pqCollaborationPanel::getSMCollaborationManager()
{
  pqCollaborationManager* pqCollab = this->getCollaborationManager();
  if (pqCollab)
  {
    if (this->Internal->LastSeenCollaborationManager != pqCollab->activeCollaborationManager())
    {
      this->Internal->LastSeenCollaborationManager = pqCollab->activeCollaborationManager();
      // Update user list UI
      onUserUpdate();
    }
    return this->Internal->LastSeenCollaborationManager;
  }
  return nullptr;
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserUpdate()
{
  vtkSMCollaborationManager* collab = this->getSMCollaborationManager();
  if (!collab)
  {
    this->Internal->members->setRowCount(0);
    this->Internal->masterControl->setVisible(false);
    return;
  }
  int nbUsers = collab->GetNumberOfConnectedClients();
  this->Internal->masterControl->setVisible(collab->IsMaster());
  this->Internal->disableFurtherConnections->setChecked(collab->GetDisableFurtherConnections());
  this->Internal->connectId->setValue(collab->GetServerConnectID());

  int userId;
  QString userName;
  this->Internal->members->setRowCount(nbUsers);
  this->Internal->CameraToFollowOfUserId = collab->GetFollowedUser();
  for (int cc = 0; cc < nbUsers; cc++)
  {
    userId = collab->GetUserId(cc);
    userName = collab->GetUserLabel(userId);

    QTableWidgetItem* item = new QTableWidgetItem(userName);
    item->setData(Qt::UserRole, userId);

    // Add local user decoration
    if (userId == collab->GetUserId())
    {
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      QFont userFont = item->font();
      userFont.setBold(true);
      userFont.setItalic(true);
      item->setFont(userFont);
    }
    else
    {
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    }

    // Add master decoration
    if (userId == collab->GetMasterId())
    {
      item->setIcon(QIcon(":/pqWidgets/Icons/pqMousePick15.png"));
    }
    else
    {
      item->setIcon(QIcon());
    }

    this->Internal->members->setItem(cc, 0, item);

    // Disable other column editing
    item = new QTableWidgetItem("");
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (userId == this->Internal->CameraToFollowOfUserId)
    {
      item->setIcon(QIcon(":/pqWidgets/Icons/pqEyeball.svg"));
    }
    this->Internal->members->setItem(cc, 1, item);

    item = new QTableWidgetItem("");
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    this->Internal->members->setItem(cc, 2, item);
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::connectViewLocalSlots(pqView* view)
{
  vtkSMRenderViewProxy* viewProxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
  if (viewProxy)
  {
    this->Internal->VTKConnector->Connect(viewProxy->GetInteractor(),
      vtkCommand::StartInteractionEvent, this, SLOT(stopFollowingCamera()));
  }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::disconnectViewLocalSlots(pqView* view)
{
  vtkSMRenderViewProxy* viewProxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
  if (viewProxy)
  {
    this->Internal->VTKConnector->Disconnect(viewProxy->GetInteractor(),
      vtkCommand::StartInteractionEvent, this, SLOT(stopFollowingCamera()));
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::stopFollowingCamera()
{
  this->followUserCamera(-1);
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onServerChanged()
{
  // update user list if real changes...
  this->getSMCollaborationManager();

  // Make sure we follow the collaboration manager once this one get created...
  if (this->Internal->NeedToConnectToCollaborationManager)
  {
    pqCollaborationManager* collab = this->getCollaborationManager();
    if (collab)
    {
      this->Internal->NeedToConnectToCollaborationManager = false;
      QObject::connect(collab, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), this,
        SLOT(writeChatMessage(pqServer*, int, QString&)));
      QObject::connect(collab, SIGNAL(triggeredUserListChanged()), this, SLOT(onUserUpdate()));

      QObject::connect(this, SIGNAL(triggerChatMessage(pqServer*, int, QString&)), collab,
        SLOT(onChatMessage(pqServer*, int, QString&)));

      QObject::connect(
        this, SIGNAL(shareLocalMousePointer(bool)), collab, SLOT(enableMousePointerSharing(bool)));

      QObject::connect(this, SIGNAL(disableFurtherConnections(bool)), collab,
        SLOT(disableFurtherConnections(bool)));

      QObject::connect(this, SIGNAL(connectIDChanged(int)), collab, SLOT(setConnectID(int)));

      QObject::connect(collab, SIGNAL(triggeredMasterUser(int)), this, SLOT(onNewMaster(int)));

      QObject::connect(collab, SIGNAL(triggerFollowCamera(int)), this, SLOT(followUserCamera(int)));

      // By default we should follow the master
      if (collab->activeCollaborationManager())
      {
        this->followUserCamera(collab->activeCollaborationManager()->GetMasterId());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onConnectIDChanged()
{
  Q_EMIT connectIDChanged(this->Internal->connectId->value());
}
