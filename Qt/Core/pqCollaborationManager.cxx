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
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "pqView.h"

#include "vtkPVServerInformation.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMProxy.h"

#include <vtkstd/set>

// Qt includes.
#include <QtDebug>
#include <QTimer>
#include <QPointer>
#include <QMap>
#include <QSignalMapper>

#define ReturnIfNotValidServer() \
        if(pqApplicationCore::instance()->getActiveServer() != this->Internals->server()) \
          {\
          cout << "Not same server" << endl;\
          return;\
          }

//***************************************************************************
//                           Internal class
//***************************************************************************
class pqCollaborationManager::pqInternals
{
public:
  pqInternals(pqCollaborationManager* owner)
    {
    this->Owner = owner;
    this->RenderingFromNotification = false;
    this->RenderTimer.setInterval(500);
    this->RenderTimer.setSingleShot(false);
    this->RenderTimer.start(1000);
    QObject::connect( &this->RenderTimer, SIGNAL(timeout()),
                      this->Owner, SLOT(render()));

    this->UpdateUserListTimer.setSingleShot(false);
    this->UpdateUserListTimer.setInterval(5000); // 5s
    this->UpdateUserListTimer.start(5000); // 5s
    QObject::connect( &this->UpdateUserListTimer, SIGNAL(timeout()),
                      this->Owner, SLOT(refreshUserList()));
    }
  //-------------------------------------------------
  void setServer(pqServer* server)
    {
    if(!this->Server.isNull())
      {
      QObject::disconnect( this->Server,
                           SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                           this->Owner, SLOT(onClientMessage(vtkSMMessage*)));
      }
    this->Server = server;
    if(server)
      {
      QObject::connect( server,
                        SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                        this->Owner, SLOT(onClientMessage(vtkSMMessage*)),
                        Qt::QueuedConnection);
      refreshInformations();
      }
    }
  //-------------------------------------------------
  pqServer* server()
    {
    return this->Server.data();
    }
  //-------------------------------------------------
  bool refreshInformations()
    {
    if(!this->Server.isNull())
      {
      this->Server->session()->UpdateServerInformation();
      vtkPVServerInformation* serverInfo = this->Server->getServerInformation();

      // General collaboration information
      this->UserID = serverInfo->GetClientId();
      this->NumberOfConnectedClients = serverInfo->GetNumberOfClients();

      // Local var to detect diff if any
      bool foundDifference = false;
      int currentUserId = -1;
      vtkstd::set<int> userListCopy = this->ConnectedClients;

      // Update connected clients informations
      this->ConnectedClients.clear();
      for(int cc=0; cc < this->NumberOfConnectedClients; ++cc)
        {
        currentUserId = serverInfo->GetClientId(cc);
        foundDifference = foundDifference ||
                          (userListCopy.find(currentUserId) == userListCopy.end());
        this->ConnectedClients.insert(currentUserId);
        }
      foundDifference = foundDifference || // Some client may have disapeared
                        (userListCopy.size() != this->ConnectedClients.size());

      // Return if diff were found
      return foundDifference;
      }

    return false;
    }
  //-------------------------------------------------
  bool CanTriggerRender()
    {
    return !this->RenderingFromNotification;
    }
  //-------------------------------------------------
  void Render(vtkTypeUInt32 viewId)
    {
    this->ViewToRender.insert(viewId);
    if(this->IsLocalRendering())
      {
      this->Owner->render();
      }
    }

  void StartRendering()
    {
    this->RenderingFromNotification = true;
    }
  //-------------------------------------------------
  void StopRendering()
    {
    this->RenderingFromNotification = false;
    }
  //-------------------------------------------------
  pqView* GetNextViewToRender()
    {
    if(this->ViewToRender.size() == 0)
      {
      return NULL;
      }
    int value = *this->ViewToRender.begin();
    this->ViewToRender.erase(this->ViewToRender.begin());
    pqApplicationCore* core = pqApplicationCore::instance();
    return core->getServerManagerModel()->findItem<pqView*>(value);
    }
  //-------------------------------------------------
  bool IsLocalRendering()
    {
    return true; // TODO
    }

  //-------------------------------------------------
  int GetClientId(int idx)
    {
    if(this->NumberOfConnectedClients != static_cast<int>(this->ConnectedClients.size()))
      {
      this->refreshInformations();
      }
    if(idx >= 0 && idx < this->NumberOfConnectedClients)
      {
      vtkstd::set<int>::iterator iter = this->ConnectedClients.begin();
      while(idx != 0)
        {
        idx--;
        iter++;
        }
      return *iter;
      }
    return -1;
    }

public:
  bool RenderingFromNotification;
  int UserID;
  int NumberOfConnectedClients;
  QMap<int, QString> UserNameMap;

protected:
  vtkstd::set<int> ConnectedClients;
  vtkstd::set<vtkTypeUInt32> ViewToRender;
  QTimer RenderTimer;
  QTimer UpdateUserListTimer;
  QPointer<pqServer> Server;
  QPointer<pqCollaborationManager> Owner;
};
//***************************************************************************/
pqCollaborationManager::pqCollaborationManager(QObject* parent) :
  QObject(parent)
{
  this->Internals = new pqInternals(this);
  pqApplicationCore* core = pqApplicationCore::instance();

  // View management
  this->viewsSignalMapper = new QSignalMapper(this);
  QObject::connect(this->viewsSignalMapper, SIGNAL(mapped(int)),
                   this, SIGNAL(triggerRender(int)));
  QObject::connect(this, SIGNAL(triggerRender(int)),
                   this, SLOT(onTriggerRender(int)));
  QObject::connect(core->getServerManagerModel(), SIGNAL(viewAdded(pqView*)),
                   this, SLOT(addCollaborationEventManagement(pqView*)));
  QObject::connect(core->getServerManagerModel(), SIGNAL(viewRemoved(pqView*)),
                   this, SLOT(removeCollaborationEventManagement(pqView*)));

  QObject::connect( this, SIGNAL(triggerChatMessage(int,QString&)),
                    this,        SLOT(onChatMessage(int,QString&)));
  QObject::connect( this, SIGNAL(triggerUpdateUser(int,QString&,bool)),
                    this,        SLOT(onUpdateUser(int,QString&,bool)));

  core->registerManager("COLLABORATION_MANAGER", this);
}

//-----------------------------------------------------------------------------
pqCollaborationManager::~pqCollaborationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  QObject::disconnect(core->getServerManagerModel(),
                      SIGNAL(viewAdded(pqView*)),
                      this, SLOT(addCollaborationEventManagement(pqView*)));
  QObject::disconnect(core->getServerManagerModel(),
                      SIGNAL(viewRemoved(pqView*)),
                      this, SLOT(removeCollaborationEventManagement(pqView*)));
  QObject::disconnect( this, SIGNAL(triggerChatMessage(int,QString&)),
                       this,        SLOT(onChatMessage(int,QString&)));
  QObject::disconnect( this, SIGNAL(triggerUpdateUser(int,QString&,bool)),
                       this,        SLOT(onUpdateUser(int,QString&,bool)));

  delete this->Internals;

}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onClientMessage(vtkSMMessage* msg)
{
  if(msg->HasExtension(QtEvent::type))
    {
    int userId = 0;
    QString userName;
    QString chatMsg;
    vtkTypeUInt32 proxyId = msg->GetExtension(QtEvent::proxy);
    switch(msg->GetExtension(QtEvent::type))
      {
      case QtEvent::RENDER:
        this->Internals->Render(proxyId);
        break;
      case QtEvent::FOCUS_PROPERTY:
        break;
      case QtEvent::FOCUS_DISPLAY:
        break;
      case QtEvent::FOCUS_INFORMATION:
        break;
      case QtEvent::ACTIVE_SOURCE:
        break;
      case QtEvent::PROXY_STATE_INVALID:
        break;
      case QtEvent::USER:
        userId = msg->GetExtension(ClientInformation::user);
        userName = msg->GetExtension(ClientInformation::name).c_str();
        emit triggerUpdateUser(userId, userName, false);
        if(msg->GetExtension(ClientInformation::req_update))
          {
          userId = this->userId();
          userName = this->getUserName(userId);
          this->onUpdateUser(userId, userName, false);
          }
        break;
      case QtEvent::CHAT:
        userId = msg->GetExtension(ClientInformation::user);
        userName = msg->GetExtension(ClientInformation::name).c_str();
        chatMsg =  msg->GetExtension(ChatMessage::txt).c_str();
        emit triggerUpdateUser(userId, userName, false);
        emit triggerChatMessage(userId, chatMsg);
        break;
      case QtEvent::OTHER:
        // Custom handling
        break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::onTriggerRender(int viewId)
{
  ReturnIfNotValidServer();
  if(this->Internals->CanTriggerRender())
    {
    // Build message to notify other clients to trigger a render for the given view
    vtkSMMessage msg;
    msg.SetExtension(QtEvent::type, QtEvent::RENDER);
    msg.SetExtension(QtEvent::proxy, viewId);

    // Broadcast the message
    this->Internals->server()->sendToOtherClients(&msg);
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onChatMessage(int userId, QString& msgContent)
{
  ReturnIfNotValidServer();

  // Broadcast to others only if its our message
  if(userId == this->Internals->UserID)
    {
    vtkSMMessage chatMsg;
    chatMsg.SetExtension(QtEvent::type, QtEvent::CHAT);
    chatMsg.SetExtension( ClientInformation::user, userId );
    chatMsg.SetExtension( ClientInformation::name,
                          this->getUserName(userId).toStdString() );
    chatMsg.SetExtension( ChatMessage::txt, msgContent.toStdString() );

    this->Internals->server()->sendToOtherClients(&chatMsg);
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onUpdateUser( int userId, QString& userName,
                                           bool requestUpdateFromOthers)
{
  // UserId can be equal to -1 when we want to invalidate the user list
  if(userId > 0)
    {
    bool nameChanged = (this->Internals->UserNameMap[userId] != userName);
    this->Internals->UserNameMap[userId] = userName;
    if(userId == this->Internals->UserID)
      {
      // Only us should broadcast our name...
      vtkSMMessage userMsg;
      userMsg.SetExtension(QtEvent::type, QtEvent::USER);
      userMsg.SetExtension(ClientInformation::user, userId);
      userMsg.SetExtension(ClientInformation::name, userName.toStdString());
      userMsg.SetExtension(ClientInformation::req_update, requestUpdateFromOthers);

      this->Internals->server()->sendToOtherClients(&userMsg);
      }

    // Notify that the user model as change
    if(nameChanged)
      {
      this->Internals->refreshInformations();
      emit triggerUpdateUserList();
      }
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::refreshUserList()
{
  if(this->Internals->refreshInformations())
    {
    emit triggerUpdateUserList();
    }
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::addCollaborationEventManagement(pqView* view)
{
  ReturnIfNotValidServer();
  this->viewsSignalMapper->setMapping(view, view->getProxy()->GetGlobalID());
  QObject::connect(view, SIGNAL(endRender()), this->viewsSignalMapper, SLOT(map()));
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::removeCollaborationEventManagement(pqView* view)
{
  ReturnIfNotValidServer();
  QObject::disconnect(view, SIGNAL(endRender()), this->viewsSignalMapper, SLOT(map()));
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::setServer(pqServer* server)
{
  this->Internals->setServer(server);
  QString userName = this->getUserName(this->userId());
  this->onUpdateUser(this->userId(), userName, true);
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::render()
{
  pqView* view;
  while((view = this->Internals->GetNextViewToRender()) != NULL)
    {
    this->Internals->StartRendering();
    view->forceRender();
    this->Internals->StopRendering();
    }
}
//-----------------------------------------------------------------------------
int pqCollaborationManager::userId()
{
  return this->Internals->UserID;
}

//-----------------------------------------------------------------------------
void pqCollaborationManager::updateUserList()
{
  this->Internals->refreshInformations();
}

//-----------------------------------------------------------------------------
int pqCollaborationManager::getNumberOfUsers()
{
  return this->Internals->NumberOfConnectedClients;
}

//-----------------------------------------------------------------------------
QString pqCollaborationManager::getUserName(int userId)
{
  if(userId == -1)
    {
    return QString("Invalid user name");
    }
  QString name = this->Internals->UserNameMap[userId];
  if(name.isEmpty())
    {
    name = "User ";
    name+= QString::number(userId);
    this->Internals->UserNameMap[userId] = name;
    }
  return name;
}
//-----------------------------------------------------------------------------
int pqCollaborationManager::getUserId(int idx)
{
  return this->Internals->GetClientId(idx);
}
