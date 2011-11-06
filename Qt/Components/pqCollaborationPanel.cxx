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
#include <QDockWidget>
#include <QTextDocument>
#include <QTextCursor>
#include <QColor>
#include <QPointer>
#include <QCheckBox>
#include <QScrollArea>
#include <QtDebug>

//// ParaView Includes.
#include "pqCollaborationManager.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqServer.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVServerInformation.h"

#include "vtkCommand.h"
#include <vtkNew.h>
#include <vtkstd/map>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

//*****************************************************************************
#include "ui_pqCollaborationPanel.h"
class pqCollaborationPanel::pqInternal : public Ui::pqCollaborationPanel
{
public:
  int CameraToFollowOfUserId;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  vtkstd::map<int, vtkSMMessage> LocalCameraStateCache;
};
//-----------------------------------------------------------------------------
pqCollaborationPanel::pqCollaborationPanel(QWidget* p):Superclass(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->members->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
  this->Internal->members->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
  this->Internal->CameraToFollowOfUserId = -1;

  QObject::connect( this->Internal->message, SIGNAL(returnPressed()),
                    this, SLOT(onUserMessage()));

  QObject::connect(this->Internal->members, SIGNAL(itemChanged(QTableWidgetItem*)),
                   this, SLOT(itemChanged(QTableWidgetItem*)));

  QObject::connect(this->Internal->members, SIGNAL(cellDoubleClicked(int,int)),
                   this, SLOT(cellDoubleClicked(int,int)));

  QObject::connect(this->Internal->shareMousePointer, SIGNAL(clicked(bool)),
                   this, SIGNAL(shareLocalMousePointer(bool)));

  QObject::connect( this, SIGNAL(triggerChatMessage(int,QString&)),
                    this,   SLOT(writeChatMessage(int,QString&)));

  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(serverAdded(pqServer*)),
                    this, SLOT(connectLocalSlots()));

  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(aboutToRemoveServer(pqServer*)),
                    this, SLOT(disconnectLocalSlots()));

  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(viewAdded(pqView*)),
                    this, SLOT(connectViewLocalSlots(pqView*)));
  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(preViewRemoved(pqView*)),
                    this, SLOT(disconnectViewLocalSlots(pqView*)));


}

//-----------------------------------------------------------------------------
pqCollaborationPanel::~pqCollaborationPanel()
{
  QObject::disconnect( this->Internal->message, SIGNAL(returnPressed()),
                       this, SLOT(onUserMessage()));

  QObject::disconnect( this->Internal->members,
                       SIGNAL(itemChanged(QTableWidgetItem*)),
                       this,
                       SLOT(itemChanged(QTableWidgetItem*)));
  QObject::disconnect( this->Internal->members,
                       SIGNAL(cellDoubleClicked(int,int)),
                       this, SLOT(cellDoubleClicked(int,int)));

  QObject::disconnect(this->Internal->shareMousePointer, SIGNAL(clicked(bool)),
                      this, SIGNAL(shareLocalMousePointer(bool)));

  QObject::disconnect( this, SIGNAL(triggerChatMessage(int,QString&)),
                       this,   SLOT(writeChatMessage(int,QString&)));

  QObject::disconnect( pqApplicationCore::instance()->getServerManagerModel(),
                       SIGNAL(serverAdded(pqServer*)),
                       this, SLOT(connectLocalSlots()));

  QObject::disconnect( pqApplicationCore::instance()->getServerManagerModel(),
                       SIGNAL(aboutToRemoveServer(pqServer*)),
                       this, SLOT(disconnectLocalSlots()));

  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserMessage()
  {
  if(this->Internal->message->text().trimmed().length() == 0)
    {
    return;
    }

  vtkSMCollaborationManager* collab = getSMCollaborationManager();
  if(collab)
    {
    int userId = collab->GetUserId();
    QString msg = this->Internal->message->text();
    emit triggerChatMessage( userId, msg);
    this->Internal->message->clear();
    }
  }
//-----------------------------------------------------------------------------
void pqCollaborationPanel::writeChatMessage(int userId, QString& txt)
  {
  QString message = QString("<b>%1:</b> %2 <br/>\n\n").
                    arg( this->getSMCollaborationManager()->GetUserLabel(userId),
                         txt.trimmed());

  this->Internal->content->textCursor().atEnd();
  this->Internal->content->insertHtml(message);
  this->Internal->content->textCursor().atEnd();
  this->Internal->content->textCursor().movePosition(QTextCursor::End);
  this->Internal->content->ensureCursorVisible();
  }
//-----------------------------------------------------------------------------
void pqCollaborationPanel::itemChanged(QTableWidgetItem* item)
{
  if(item->column() == 0)
    {
    vtkSMCollaborationManager* collab = this->getSMCollaborationManager();
    if(collab)
      {
      int id = item->data(Qt::UserRole).toInt();
      if(collab->GetUserId() == id)
        {
        QString userName = item->text();
        if(userName != collab->GetUserLabel(id))
          {
          collab->SetUserLabel(id, userName.toAscii().data());
          }
        }
      }
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::cellDoubleClicked(int row, int col)
{
  int userId = this->Internal->members->item(row, 0)->data(Qt::UserRole).toInt();
  switch(col)
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
  if(pqCollaborationManager* collabManager = this->getCollaborationManager())
    {
    collabManager->setFollowUserView(userId);
    }

  if(this->Internal->CameraToFollowOfUserId == userId)
    {
    return;
    }

  // Update user Camera to follow
  if(this->getSMCollaborationManager()->GetUserId() == userId)
    {
    this->Internal->CameraToFollowOfUserId = 0; // Looking at our local camera
    }
  else
    {
    this->Internal->CameraToFollowOfUserId = userId;
    }

  // If we are the master we want the slaves to follow the same camera as us
  if(this->getSMCollaborationManager()->IsMaster())
    {
    this->getSMCollaborationManager()->FollowUser(userId);
    }

  // Update the UI
  int nbRows = this->Internal->members->rowCount();
  for(int i=0; i < nbRows; i++)
    {
    if(userId == this->Internal->members->item(i, 0)->data(Qt::UserRole).toInt())
      {
      this->Internal->members->item(i, 1)->setIcon(QIcon(":/pqWidgets/Icons/pqEyeball16.png"));
      }
    else
      {
      this->Internal->members->item(i, 1)->setIcon(QIcon());
      }
    }

  vtkstd::map<int, vtkSMMessage>::iterator camCache;
  camCache = this->Internal->LocalCameraStateCache.find(userId);
  if(camCache != this->Internal->LocalCameraStateCache.end())
    {
    this->onShareOnlyMessage(&camCache->second);
    }

  // If we follow master lets selection model follow as well
  bool followMaster = (userId == this->getSMCollaborationManager()->GetMasterId());
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  for(vtkIdType idx=0; idx < pxm->GetNumberOfSelectionModel(); idx++)
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
  for(int i=0; i < nbRows; i++)
    {
    if(masterId == this->Internal->members->item(i, 0)->data(Qt::UserRole).toInt())
      {
      this->Internal->members->item(i, 0)->setIcon(QIcon(":/pqWidgets/Icons/pqMousePick15.png"));
      }
    else
      {
      this->Internal->members->item(i, 0)->setIcon(QIcon());
      }
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::promoteToMaster(int masterId)
{
  if(this->getSMCollaborationManager())
    {
    vtkSMCollaborationManager* collabManager = this->getSMCollaborationManager();
    if(collabManager->GetMasterId() == collabManager->GetUserId())
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
  if(pqCollab)
    {
    return pqCollab->collaborationManager();
    }
  return NULL;
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserUpdate()
{

  vtkSMCollaborationManager* collab = this->getSMCollaborationManager();
  if(!collab)
    {
    return;
    }
  int nbUsers = collab->GetNumberOfConnectedClients();
  int userId;
  QString userName;
  this->Internal->members->setRowCount(nbUsers);
  for(int cc = 0; cc < nbUsers; cc++)
    {
    userId = collab->GetUserId(cc);
    userName = collab->GetUserLabel(userId);

    QTableWidgetItem* item = new QTableWidgetItem(userName);
    item->setData(Qt::UserRole, userId);

    // Add local user decoration
    if(userId == collab->GetUserId())
      {
      item->setFlags( item->flags() | Qt::ItemIsEditable );
      QFont userFont = item->font();
      userFont.setBold(true);
      userFont.setItalic(true);
      item->setFont(userFont);
      }
    else
      {
      item->setFlags( item->flags() & ~Qt::ItemIsEditable );
      }

    // Add master decoration
    if(userId == collab->GetMasterId())
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
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    if(userId == this->Internal->CameraToFollowOfUserId)
      {
      item->setIcon(QIcon(":/pqWidgets/Icons/pqEyeball16.png"));
      }
    this->Internal->members->setItem(cc, 1, item);

    item = new QTableWidgetItem("");
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    this->Internal->members->setItem(cc, 2, item);
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::connectLocalSlots()
{
  pqCollaborationManager* collab = this->getCollaborationManager();
  if(collab)
    {
    QObject::connect( collab, SIGNAL(triggerChatMessage(int,QString&)),
                      this,   SLOT(writeChatMessage(int,QString&)));
    QObject::connect( collab,
                      SIGNAL(triggeredUserListChanged()),
                      this,   SLOT(onUserUpdate()));
    QObject::connect( collab,
                      SIGNAL(triggerStateClientOnlyMessage(vtkSMMessage*)),
                      this,   SLOT(onShareOnlyMessage(vtkSMMessage*)));

    QObject::connect( this,   SIGNAL(triggerChatMessage(int,QString&)),
                      collab, SLOT(onChatMessage(int,QString&)));

    QObject::connect( this,   SIGNAL(shareLocalMousePointer(bool)),
                      collab, SLOT(enableMousePointerSharing(bool)));

    QObject::connect( collab, SIGNAL(triggeredMasterUser(int)),
                      this,   SLOT(onNewMaster(int)));

    QObject::connect( collab, SIGNAL(triggerFollowCamera(int)),
                      this,   SLOT(followUserCamera(int)));

    // Update the graphical panel
    collab->collaborationManager()->UpdateUserInformations();
    onUserUpdate(); // This may be called twice (one here + one from emit)

    QDockWidget* parentDock = qobject_cast<QDockWidget*>(this->parentWidget());
    if(parentDock)
      {
      parentDock->toggleViewAction()->setEnabled(true);
      QObject::connect( parentDock, SIGNAL(visibilityChanged(bool)),
                        this,         SLOT(onUserUpdate()));
      }

    // By default we should follow the master
    this->followUserCamera(collab->collaborationManager()->GetMasterId());
    }
  else
    {
    QDockWidget* parentDock = qobject_cast<QDockWidget*>(this->parentWidget());
    if(parentDock)
      {
      parentDock->hide();
      parentDock->toggleViewAction()->setEnabled(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::disconnectLocalSlots()
{
  pqCollaborationManager* collab = this->getCollaborationManager();
  if(collab)
    {
    // If we disconnect, this means a new session has been created so all
    // objects that we observed have been deleted
    QObject::disconnect( collab, SIGNAL(triggerChatMessage(int,QString&)),
                         this,   SLOT(writeChatMessage(int,QString&)));
    QObject::disconnect( collab,
                         SIGNAL(triggeredUserListChanged()),
                         this,   SLOT(onUserUpdate()));
    QObject::disconnect( collab,
                         SIGNAL(triggerStateClientOnlyMessage(vtkSMMessage*)),
                         this,   SLOT(onShareOnlyMessage(vtkSMMessage*)));

    QObject::disconnect( this,   SIGNAL(triggerChatMessage(int,QString&)),
                         collab, SLOT(onChatMessage(int,QString&)));

    QObject::disconnect( this,   SIGNAL(shareLocalMousePointer(bool)),
                         collab, SLOT(enableMousePointerSharing(bool)));

    QObject::disconnect( collab, SIGNAL(triggeredMasterUser(int)),
                         this,   SLOT(onNewMaster(int)));

    QObject::disconnect( collab, SIGNAL(triggerFollowCamera(int)),
                         this,   SLOT(followUserCamera(int)));

    QDockWidget* parentDock = qobject_cast<QDockWidget*>(this->parentWidget());
    if(parentDock)
      {
      QObject::disconnect( parentDock, SIGNAL(visibilityChanged(bool)),
                           this,         SLOT(onUserUpdate()));
      }
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::connectViewLocalSlots(pqView* view)
{
  vtkSMRenderViewProxy* viewProxy =
      vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
  if(viewProxy)
    {
    this->Internal->VTKConnector->Connect( viewProxy->GetInteractor(),
                                           vtkCommand::StartInteractionEvent,
                                           this, SLOT(stopFollowingCamera()));
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::disconnectViewLocalSlots(pqView* view)
{
  vtkSMRenderViewProxy* viewProxy =
      vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
  if(viewProxy)
    {
    this->Internal->VTKConnector->Disconnect( viewProxy->GetInteractor(),
                                              vtkCommand::StartInteractionEvent,
                                              this, SLOT(stopFollowingCamera()));
    }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::stopFollowingCamera()
{
  this->followUserCamera(-1);
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onShareOnlyMessage(vtkSMMessage *msg)
{
  // Check if its camera update
  if(msg->HasExtension(DefinitionHeader::client_class) &&
     msg->GetExtension(DefinitionHeader::client_class) == "vtkSMCameraProxy")
    {
    int currentUserId = static_cast<int>(msg->client_id());

    // Keep in cache the latest camera position of each participants
    this->Internal->LocalCameraStateCache[currentUserId].CopyFrom(*msg);

    // If I'm following that one just update my camera
    if(this->Internal->CameraToFollowOfUserId == currentUserId)
      {
      vtkTypeUInt32 cameraId = msg->global_id();
      pqApplicationCore* core = pqApplicationCore::instance();
      vtkSMProxyLocator* locator =
          core->getActiveServer()->session()->GetProxyLocator();
      vtkSMProxy* proxy = locator->LocateProxy(cameraId);
      if(proxy)
        {
        assert("Impossible to update camera as the session is in ProcessingRemoteNotification state" &&
               !proxy->GetSession()->IsProcessingRemoteNotification());
        // Update Proxy
        proxy->EnableLocalPushOnly();
        proxy->LoadState(msg, locator);
        proxy->UpdateVTKObjects();
        proxy->DisableLocalPushOnly();
        core->render();
        }
      }
    }
}
