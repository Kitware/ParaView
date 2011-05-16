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
#include <QTextDocument>
#include <QTextCursor>
#include <QColor>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>

//// ParaView Includes.
#include "pqCollaborationManager.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqServer.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRenderViewProxy.h"
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
};
//-----------------------------------------------------------------------------
pqCollaborationPanel::pqCollaborationPanel(QWidget* p):Superclass(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->CameraToFollowOfUserId = -1;

  QObject::connect( this->Internal->message, SIGNAL(returnPressed()),
                    this, SLOT(onUserMessage()));

  QObject::connect(this->Internal->members, SIGNAL(itemChanged(QTableWidgetItem*)),
                   this, SLOT(itemChanged(QTableWidgetItem*)));

  QObject::connect(this->Internal->members, SIGNAL(cellDoubleClicked(int,int)),
                   this, SLOT(cellDoubleClicked(int,int)));

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

  QObject::disconnect( this, SIGNAL(triggerChatMessage(int,QString&)),
                       this,   SLOT(writeChatMessage(int,QString&)));
  QObject::disconnect( this, SIGNAL(triggerUpdateUser(int,QString&,bool)),
                       this,   SLOT(onUserUpdate()));

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

  pqCollaborationManager* collab = getCollaborationManager();
  if(collab)
    {
    int userId = collab->userId();
    QString msg = this->Internal->message->text();
    emit triggerChatMessage( userId, msg);
    this->Internal->message->clear();
    }
  }
//-----------------------------------------------------------------------------
void pqCollaborationPanel::writeChatMessage(int userId, QString& txt)
  {
  QString message = QString("<b>%1:</b> %2 <br/>\n\n").
                    arg( this->getCollaborationManager()->getUserName(userId),
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
    pqCollaborationManager* collab = this->getCollaborationManager();
    if(collab)
      {
      int id = item->data(Qt::UserRole).toInt();
      if(collab->userId() == id)
        {
        QString userName = item->text();
        if(userName != collab->getUserName(id))
          {
          emit triggerUpdateUser(id, userName, true);
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
      this->setCameraSynchronizationToUser(userId);
      break;
    case 2: // 3D Pointer
      break;
    }
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::setCameraSynchronizationToUser(int userId)
{
  if(this->Internal->CameraToFollowOfUserId == userId)
    {
    return;
    }

  // Update user Camera to follow
  if(this->getCollaborationManager()->userId() == userId)
    {
    this->Internal->CameraToFollowOfUserId = 0; // Looking at our local camera
    }
  else
    {
    this->Internal->CameraToFollowOfUserId = userId;
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
}

//-----------------------------------------------------------------------------
pqCollaborationManager* pqCollaborationPanel::getCollaborationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  return qobject_cast<pqCollaborationManager*>(core->manager("COLLABORATION_MANAGER"));
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserUpdate()
{
  pqCollaborationManager* collab = this->getCollaborationManager();
  if(!collab)
    {
    return;
    }
  int nbUsers = collab->getNumberOfUsers();
  int userId;
  QString userName;
  this->Internal->members->setRowCount(nbUsers);
  for(int cc = 0; cc < nbUsers; cc++)
    {
    userId = collab->getUserId(cc);
    userName = collab->getUserName(userId);

    QTableWidgetItem* item = new QTableWidgetItem(userName);
    item->setData(Qt::UserRole, userId);
    if(userId == collab->userId())
      {
      item->setFlags( item->flags() | Qt::ItemIsEditable );
      QFont font = item->font();
      font.setBold(true);
      font.setItalic(true);
      item->setFont(font);
      }
    else
      {
      item->setFlags( item->flags() & ~Qt::ItemIsEditable );
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

  this->Internal->members->horizontalHeader()->resizeSections(QHeaderView::Stretch);
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::connectLocalSlots()
{
  pqCollaborationManager* collab = this->getCollaborationManager();
  if(collab)
    {
    QObject::connect( collab, SIGNAL(triggerChatMessage(int,QString&)),
                      this,   SLOT(writeChatMessage(int,QString&)));
    QObject::connect( collab, SIGNAL(triggerUpdateUser(int,QString&,bool)),
                      this,   SLOT(onUserUpdate()));
    QObject::connect( collab, SIGNAL(triggerUpdateUserList()),
                      this,   SLOT(onUserUpdate()));
    QObject::connect( collab, SIGNAL(triggerStateClientOnlyMessage(vtkSMMessage*)),
                      this,   SLOT(onShareOnlyMessage(vtkSMMessage*)));

    QObject::connect( this,   SIGNAL(triggerChatMessage(int,QString&)),
                      collab, SLOT(onChatMessage(int,QString&)));
    QObject::connect( this,   SIGNAL(triggerUpdateUser(int,QString&,bool)),
                      collab, SLOT(onUpdateUser(int,QString&,bool)));

    // Update the graphical panel
    QString userName = collab->getUserName(collab->userId());
    emit triggerUpdateUser( collab->userId(), userName, true);
    onUserUpdate(); // This may be called twice (one here + one from emit)
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
    QObject::disconnect( collab, SIGNAL(triggerUpdateUser(int,QString&,bool)),
                         this,   SLOT(onUserUpdate()));
    QObject::disconnect( collab, SIGNAL(triggerUpdateUserList()),
                         this,   SLOT(onUserUpdate()));
    QObject::disconnect( collab, SIGNAL(triggerStateClientOnlyMessage(vtkSMMessage*)),
                         this,   SLOT(onShareOnlyMessage(vtkSMMessage*)));

    QObject::disconnect( this,   SIGNAL(triggerChatMessage(int,QString&)),
                         collab, SLOT(onChatMessage(int,QString&)));
    QObject::disconnect( this,   SIGNAL(triggerUpdateUser(int,QString&,bool)),
                         collab, SLOT(onUpdateUser(int,QString&,bool)));
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
  this->setCameraSynchronizationToUser(-1);
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onShareOnlyMessage(vtkSMMessage *msg)
{
  // Check if its camera update
  if(msg->HasExtension(DefinitionHeader::client_class) &&
     msg->GetExtension(DefinitionHeader::client_class) == "vtkSMCameraProxy")
    {
    if(this->Internal->CameraToFollowOfUserId == static_cast<int>(msg->client_id()))
      {
      vtkTypeUInt32 cameraId = msg->global_id();
      pqApplicationCore* core = pqApplicationCore::instance();
      vtkSMProxyLocator* locator =
          core->getActiveServer()->session()->GetProxyLocator();
      vtkSMProxy* proxy = locator->LocateProxy(cameraId);
      if(proxy)
        {
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
