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
#include "vtkSMProxy.h"

#include <vtkstd/set>

// Qt includes.
#include <QtDebug>
#include <QTimer>
#include <QPointer>
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
      }
    this->Server = server;
    if(server)
      {
      QObject::connect( server,
                        SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                        this->Owner, SLOT(onClientMessage(vtkSMMessage*)),
                        Qt::QueuedConnection);
      }
    }
  //-------------------------------------------------
  pqServer* server()
    {
    return this->Server.data();
    }
  //-------------------------------------------------
  void refreshInformations()
    {
    if(!this->Server.isNull())
      {
      vtkPVServerInformation* serverInfo = this->Server->getServerInformation();

      // General collaboration information
      this->ClientID = serverInfo->GetClientId();
      this->NumberOfConnectedClients = serverInfo->GetNumberOfClients();

      // Update connected clients informations
      this->ConnectedClients.clear();
      for(int cc=0; cc < this->NumberOfConnectedClients; ++cc)
        {
        this->ConnectedClients.insert(serverInfo->GetClientId(cc));
        }
      }
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
private:
  bool RenderingFromNotification;
  int ClientID;
  int NumberOfConnectedClients;
  vtkstd::set<int> ConnectedClients;
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

  delete this->Internals;

}
//-----------------------------------------------------------------------------
void pqCollaborationManager::onClientMessage(vtkSMMessage* msg)
{
  if(msg->HasExtension(QtEvent::type))
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    vtkTypeUInt32 proxyId = msg->GetExtension(QtEvent::proxy);
    switch(msg->GetExtension(QtEvent::type))
      {
      case QtEvent::RENDER:
        this->Internals->Render(proxyId);
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
