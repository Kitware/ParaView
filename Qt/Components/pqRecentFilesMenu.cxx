/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.cxx

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

#include "pqRecentFilesMenu.h"
#include "pqSimpleServerStartup.h"

#include <pqApplicationCore.h>
#include <pqServerResource.h>
#include <pqServerResources.h>

#include <QMenu>
#include <QTimer>
#include <QtDebug>

#include <vtkstd/algorithm>

/////////////////////////////////////////////////////////////////////////////
// pqRecentFilesMenu::pqImplementation

class pqRecentFilesMenu::pqImplementation
{
public:
  pqImplementation(QMenu& menu) :
    Menu(menu)
  {
  }
  
  ~pqImplementation()
  {
  }

  QMenu& Menu;
  pqServerResource RecentResource;
  pqSimpleServerStartup ServerStartup;
  
  /// Functor that returns true if two resources have the same URI scheme and host(s)
  class SameSchemeAndHost
  {
  public:
    SameSchemeAndHost(const pqServerResource& lhs) :
      LHS(lhs)
    {
    }
    
    bool operator()(const pqServerResource& rhs) const
    {
      return this->LHS.schemeHosts() == rhs.schemeHosts();
    }
    
  private:
    const pqServerResource& LHS;
  };
};

/////////////////////////////////////////////////////////////////////////////
// pqRecentFilesMenu

pqRecentFilesMenu::pqRecentFilesMenu(QMenu& menu, QObject* p) :
  QObject(p), Implementation(new pqImplementation(menu))
{
  connect(
    &pqApplicationCore::instance()->serverResources(),
    SIGNAL(changed()),
    this,
    SLOT(onResourcesChanged()));
  
  connect(
    &this->Implementation->Menu,
    SIGNAL(triggered(QAction*)),
    this,
    SLOT(onOpenResource(QAction*)));
    
  connect(
    &this->Implementation->ServerStartup,
    SIGNAL(serverStarted(pqServer*)),
    this,
    SLOT(onServerStarted(pqServer*)));

  QObject::connect(
    &this->Implementation->ServerStartup, SIGNAL(serverFailed()),
    this, SIGNAL(serverConnectFailed()));
  
  this->onResourcesChanged();
}

pqRecentFilesMenu::~pqRecentFilesMenu()
{
  delete this->Implementation;
}

void pqRecentFilesMenu::onResourcesChanged()
{
  this->Implementation->Menu.clear();
  
  // Get the set of all resources in most-recently-used order ...
  pqServerResources::ListT resources = 
    pqApplicationCore::instance()->serverResources().list();

  // Get the set of servers with unique scheme/host in most-recently-used order ...
  pqServerResources::ListT servers;
  for(int i = 0; i != resources.size(); ++i)
    {
    pqServerResource resource = resources[i];
    pqServerResource server = resource.scheme() == "session" ?
      resource.sessionServer().schemeHostsPorts() :
      resource.schemeHostsPorts();
      
    // If this host isn't already in the list, add it ...
    if(!vtkstd::count_if(servers.begin(), servers.end(), pqImplementation::SameSchemeAndHost(server)))
      {
      servers.push_back(server);
      }
    }

  // Display the servers ...
  for(int i = 0; i != servers.size(); ++i)
    {
    const pqServerResource& server = servers[i];
    
    const QString label = server.schemeHosts().toURI();
    
    QAction* const action = new QAction(label, &this->Implementation->Menu);
    action->setData(server.serializeString());
    
    action->setIcon(QIcon(":/pqWidgets/Icons/pqConnect16.png"));
    
    QFont font = action->font();
    font.setBold(true);
    action->setFont(font);
    
    this->Implementation->Menu.addAction(action);
    
    // Display sessions associated with the server first ...
    for(int j = 0; j != resources.size(); ++j)
      {
      const pqServerResource& resource = resources[j];

      if(
        resource.scheme() != "session"
        || resource.path().isEmpty()
        || resource.sessionServer().schemeHosts() != server.schemeHosts())
        {
        continue;
        }
        
      QAction* const act = new QAction(resource.path(), &this->Implementation->Menu);
      act->setData(resource.serializeString());
      act->setIcon(QIcon(":/pqWidgets/Icons/pqAppIcon16.png"));
      
      this->Implementation->Menu.addAction(act);
      }
    
    // Display files associated with the server next ...
    for(int j = 0; j != resources.size(); ++j)
      {
      const pqServerResource& resource = resources[j];

      if(
        resource.scheme() == "session"
        || resource.path().isEmpty()
        || resource.schemeHosts() != server.schemeHosts())
        {
        continue;
        }
        
      QAction* const act = new QAction(resource.path(), &this->Implementation->Menu);
      act->setData(resource.serializeString());

      this->Implementation->Menu.addAction(act);
      }
    }
}

void pqRecentFilesMenu::onOpenResource(QAction* action)
{
  // Note: we can't update the resources here because it would destroy the
  // action that's calling this slot.  So, schedule an update for the
  // next time the UI is idle.
  this->Implementation->RecentResource =
    pqServerResource(action->data().toString());
  QTimer::singleShot(0, this, SLOT(onOpenResource()));
}

void pqRecentFilesMenu::onOpenResource()
{
  const pqServerResource resource = this->Implementation->RecentResource;
  
  const pqServerResource server =
    resource.scheme() == "session"
      ? resource.sessionServer().schemeHostsPorts()
      : resource.schemeHostsPorts();

  this->Implementation->ServerStartup.startServer(server);
}

void pqRecentFilesMenu::onServerStarted(pqServer* server)
{
  pqServerResources& resources = pqApplicationCore::instance()->serverResources();
  resources.open(server, this->Implementation->RecentResource);
  resources.add(this->Implementation->RecentResource);
  resources.save(*pqApplicationCore::instance()->settings());
}
