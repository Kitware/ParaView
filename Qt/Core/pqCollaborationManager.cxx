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
#include "pqPipelineSource.h"

#include "vtkPVMultiClientsInformation.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMProxy.h"

#include <vtkstd/set>
#include <vtkWeakPointer.h>

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
    }
  //-------------------------------------------------
  void setServer(pqServer* server)
    {
    if(!this->Server.isNull())
      {
      QObject::disconnect( this->Server,
                           SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                           this->Owner, SLOT(onClientMessage(vtkSMMessage*)));
      QObject::disconnect( this->Server,
                           SIGNAL(triggeredMasterUser(int)),
                           this->Owner, SIGNAL(triggeredMasterUser(int)));
      QObject::disconnect( this->Server,
                           SIGNAL(triggeredUserListChanged()),
                           this->Owner, SIGNAL(triggeredUserListChanged()));
      QObject::disconnect( this->Server,
                           SIGNAL(triggeredUserName(int, QString&)),
                           this->Owner, SIGNAL(triggeredUserName(int, QString&)));

      }
    this->Server = server;
    if(server)
      {
      QObject::connect( server,
                        SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                        this->Owner, SLOT(onClientMessage(vtkSMMessage*)),
                        Qt::QueuedConnection);
      QObject::connect( server,
                        SIGNAL(triggeredMasterUser(int)),
                        this->Owner, SIGNAL(triggeredMasterUser(int)));
      QObject::connect( server,
                        SIGNAL(triggeredUserListChanged()),
                        this->Owner, SIGNAL(triggeredUserListChanged()));
      QObject::connect( server,
                        SIGNAL(triggeredUserName(int, QString&)),
                        this->Owner, SIGNAL(triggeredUserName(int, QString&)));
      if(vtkSMSessionClient::SafeDownCast(server->session()))
        {
        vtkSMSessionClient* session =
            vtkSMSessionClient::SafeDownCast(server->session());
        this->CollaborationManager = session->GetCollaborationManager();
        this->CollaborationManager->UpdateUserInformations();
        }
      else
        {
        this->CollaborationManager = NULL;
        }
      }
    }

  //-------------------------------------------------
  pqServer* server()
    {
    return this->Server.data();
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
    if(this->CollaborationManager)
      {
      return this->CollaborationManager->GetUserId(idx);
      }
    return -1;
    }

public:
  bool RenderingFromNotification;
  QMap<int, QString> UserNameMap;
  vtkWeakPointer<vtkSMCollaborationManager> CollaborationManager;

protected:
  vtkstd::set<vtkTypeUInt32> ViewToRender;
  QTimer RenderTimer;
  QPointer<pqServer> Server;
  QPointer<pqCollaborationManager> Owner;
};
//***************************************************************************/
pqCollaborationManager::pqCollaborationManager(QObject* parent) :
  QObject(parent)
{
  this->Internals = new pqInternals(this);
  pqApplicationCore* core = pqApplicationCore::instance();

  // Signal mappers
  this->viewsSignalMapper = new QSignalMapper(this);

  // View management
  QObject::connect(this->viewsSignalMapper, SIGNAL(mapped(int)),
                   this, SIGNAL(triggerRender(int)));
  QObject::connect(this, SIGNAL(triggerRender(int)),
                   this, SLOT(onTriggerRender(int)));
  QObject::connect(core->getServerManagerModel(), SIGNAL(viewAdded(pqView*)),
                   this, SLOT(addCollaborationEventManagement(pqView*)));
  QObject::connect(core->getServerManagerModel(), SIGNAL(viewRemoved(pqView*)),
                   this, SLOT(removeCollaborationEventManagement(pqView*)));

  // Chat management + User list panel
  QObject::connect( this, SIGNAL(triggerChatMessage(int,QString&)),
                    this,        SLOT(onChatMessage(int,QString&)));

  core->registerManager("COLLABORATION_MANAGER", this);
}

//-----------------------------------------------------------------------------
pqCollaborationManager::~pqCollaborationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  // View management
  QObject::disconnect(core->getServerManagerModel(),
                      SIGNAL(viewAdded(pqView*)),
                      this, SLOT(addCollaborationEventManagement(pqView*)));
  QObject::disconnect(core->getServerManagerModel(),
                      SIGNAL(viewRemoved(pqView*)),
                      this, SLOT(removeCollaborationEventManagement(pqView*)));

  // Chat management + User list panel
  QObject::disconnect( this, SIGNAL(triggerChatMessage(int,QString&)),
                       this, SLOT(onChatMessage(int,QString&)));

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
        //this->Internals->Render(proxyId);
        break;
      case QtEvent::INSPECTOR_TAB:
        // We use proxyId as holder of tab index
        emit triggerInspectorSelectedTabChanged(proxyId);;
        break;
      case QtEvent::PROXY_STATE_INVALID:
        break;
      case QtEvent::CHAT:
        userId = msg->GetExtension(ChatMessage::author);
        userName = this->Internals->CollaborationManager->GetUserName(userId);
        chatMsg =  msg->GetExtension(ChatMessage::txt).c_str();
        emit triggerChatMessage(userId, chatMsg);
        break;
      case QtEvent::OTHER:
        // Custom handling
        break;
      }
    }
  else
    {
    emit triggerStateClientOnlyMessage(msg);
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
  if(userId == this->Internals->CollaborationManager->GetUserId())
    {
    vtkSMMessage chatMsg;
    chatMsg.SetExtension(QtEvent::type, QtEvent::CHAT);
    chatMsg.SetExtension( ChatMessage::author,
                          this->Internals->CollaborationManager->GetUserId());
    chatMsg.SetExtension( ChatMessage::txt, msgContent.toStdString() );

    this->Internals->server()->sendToOtherClients(&chatMsg);
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onInspectorSelectedTabChanged(int tabIndex)
{
  ReturnIfNotValidServer();
  vtkSMMessage activeTab;
  activeTab.SetExtension(QtEvent::type, QtEvent::INSPECTOR_TAB);
  // We use proxyId as holder of tab index
  activeTab.SetExtension(QtEvent::proxy, tabIndex);

  this->Internals->server()->sendToOtherClients(&activeTab);
}

//-----------------------------------------------------------------------------
vtkSMCollaborationManager* pqCollaborationManager::collaborationManager()
{
  return this->Internals->CollaborationManager;
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
}
//-----------------------------------------------------------------------------
pqServer* pqCollaborationManager::server()
{
  return this->Internals->server();
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
