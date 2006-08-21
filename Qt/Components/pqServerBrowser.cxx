/*=========================================================================

   Program: ParaView
   Module:    pqServerBrowser.cxx

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

#include "pqServerBrowser.h"
#include "pqSimpleServerStartup.h"
#include "ui_pqServerBrowser.h"

#include <pqApplicationCore.h>
#include <pqServerResource.h>
#include <pqServerResources.h>

#include <QtDebug>

//////////////////////////////////////////////////////////////////////////////
// pqServerBrowser::pqImplementation

class pqServerBrowser::pqImplementation
{
public:
  pqImplementation() :
    ServerStartup(
      *pqApplicationCore::instance()->settings(),
      pqApplicationCore::instance()->serverStartups())  
  {
  }

  Ui::pqServerBrowser UI;
  
  pqServerResource Server;
  pqSimpleServerStartup ServerStartup;
  pqServer* ConnectedServer;
};

//////////////////////////////////////////////////////////////////////////////
// pqServerBrowser

pqServerBrowser::pqServerBrowser(QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);
  this->setObjectName("ServerBrowser");
  this->setWindowTitle(tr("Pick Server:"));
  
  QObject::connect(
    this->Implementation->UI.serverType,
    SIGNAL(activated(int)),
    this,
    SLOT(onServerTypeActivated(int)));

  this->Implementation->UI.serverType->setCurrentIndex(0);
  this->onServerTypeActivated(0);

  this->Implementation->ConnectedServer=NULL;
  
  connect(
    &this->Implementation->ServerStartup,
    SIGNAL(serverCancelled()),
    this,
    SLOT(onServerCancelled()));
  
  connect(
    &this->Implementation->ServerStartup,
    SIGNAL(serverFailed()),
    this,
    SLOT(onServerFailed()));
  
  connect(
    &this->Implementation->ServerStartup,
    SIGNAL(serverStarted(pqServer*)),
    this,
    SLOT(onServerStarted(pqServer*)));
}

pqServerBrowser::~pqServerBrowser()
{
  delete this->Implementation;
}
pqServer* pqServerBrowser::getConnectedServer()
{
  return this->Implementation->ConnectedServer;
}

void pqServerBrowser::onServerTypeActivated(int Index)
{
  switch(Index)
    {
    case 0:
      this->Implementation->UI.stackedWidget->setCurrentIndex(0);
      break;
    case 1:
    case 2:
      this->Implementation->UI.stackedWidget->setCurrentIndex(1);
      break;
    case 3:
    case 4:
      this->Implementation->UI.stackedWidget->setCurrentIndex(2);
      break;
    }
}

void pqServerBrowser::accept()
{
  pqServerResource server;
  
  switch(this->Implementation->UI.serverType->currentIndex())
    {
    case 0:
      server.setScheme("builtin");
      break;
      
    case 1:
      server.setScheme("cs");
      server.setHost(this->Implementation->UI.host->text());
      server.setPort(this->Implementation->UI.port->value());
      break;

    case 2:
      server.setScheme("csrc");
      server.setHost(this->Implementation->UI.host->text());
      server.setPort(this->Implementation->UI.port->value());
      break;

    case 3:
      server.setScheme("cdsrs");
      server.setDataServerHost(this->Implementation->UI.dataServerHost->text());
      server.setDataServerPort(this->Implementation->UI.dataServerPort->value());
      server.setRenderServerHost(this->Implementation->UI.renderServerHost->text());
      server.setRenderServerPort(this->Implementation->UI.renderServerPort->value());
      break;
    
    case 4:
      server.setScheme("cdsrsrc");
      server.setDataServerHost(this->Implementation->UI.dataServerHost->text());
      server.setDataServerPort(this->Implementation->UI.dataServerPort->value());
      server.setRenderServerHost(this->Implementation->UI.renderServerHost->text());
      server.setRenderServerPort(this->Implementation->UI.renderServerPort->value());
      break;

    default:
      qCritical() << "Unknown server type";
      Superclass::accept();
      return;
    }
  
  this->Implementation->Server = server;
  this->Implementation->ServerStartup.startServer(server);
}

void pqServerBrowser::onServerCancelled()
{
  Superclass::accept();
}

void pqServerBrowser::onServerFailed()
{
  Superclass::accept();
}

void pqServerBrowser::onServerStarted(pqServer* server)
{
  if(server)
    {
    pqServerResources& resources =
      pqApplicationCore::instance()->serverResources();

    resources.add(this->Implementation->Server);

    this->Implementation->ConnectedServer=server;
    emit this->serverConnected(server);
    }

  Superclass::accept();
}
