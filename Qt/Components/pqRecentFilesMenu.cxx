/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include <pqApplicationCore.h>
#include <pqServerResources.h>

#include <QMenu>
#include <QTimer>
#include <QtDebug>

#include <algorithm>

class pqRecentFilesMenu::pqImplementation
{
public:
  pqImplementation(QMenu& menu) :
    Menu(menu)
  {
  }

  QMenu& Menu;
  pqServerResource RecentResource;
};

pqRecentFilesMenu::pqRecentFilesMenu(QMenu& menu) :
  Implementation(new pqImplementation(menu))
{
  connect(&pqApplicationCore::instance()->serverResources(),
    SIGNAL(changed()), this, SLOT(onResourcesChanged()));
  
  connect(&this->Implementation->Menu, SIGNAL(triggered(QAction*)),
    this, SLOT(onOpenResource(QAction*)));
  
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

  // Get a list of servers in most-recently-used order ...
  pqServerResources::ListT servers;
  for(int i = 0; i != resources.size(); ++i)
    {
    pqServerResource server = resources[i].server();
    if(-1 == servers.indexOf(server))
      {
      servers.push_back(server);
      }
    }

  // Display the resources, grouped by server ...
  for(int i = 0; i != servers.size(); ++i)
    {
    const pqServerResource& server = servers[i];
    
    QString label = server.scheme() == "builtin" ? "builtin" : server.toString();
    
    QAction* const action = new QAction(label, &this->Implementation->Menu);
    action->setData(server.toString());
    
    action->setIcon(QIcon(":/pqWidgets/Icons/pqConnect16.png"));
    
    QFont font = action->font();
    font.setBold(true);
    action->setFont(font);
    
    this->Implementation->Menu.addAction(action);
    
    for(int j = 0; j != resources.size(); ++j)
      {
      const pqServerResource& resource = resources[j];
      
      if(resource.path().isEmpty() || resource.server() != server)
        {
        continue;
        }
        
      QAction* const action = new QAction(resource.path(), &this->Implementation->Menu);
      action->setData(resource.toString());
      
//      action->setIcon(QIcon(":/pqWidgets/Icons/pqAppIcon16.png"));
      
      this->Implementation->Menu.addAction(action);
      }
    }
}

void pqRecentFilesMenu::onOpenResource(QAction* action)
{
  this->Implementation->RecentResource = pqServerResource(action->data().toString());
  pqApplicationCore::instance()->serverResources().open(this->Implementation->RecentResource);
  
  // Note: we can't update the resources here because it would destroy the action that's calling
  // this slot.  So, schedule an update for the next time the UI is idle.
  QTimer::singleShot(0, this, SLOT(onUpdateResources()));
}

void pqRecentFilesMenu::onUpdateResources()
{
  pqApplicationCore::instance()->serverResources().add(this->Implementation->RecentResource);
}
