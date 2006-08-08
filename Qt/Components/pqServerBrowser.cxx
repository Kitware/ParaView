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

#include <QMessageBox>

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
    this->Implementation->UI.ServerType,
    SIGNAL(activated(int)),
    this,
    SLOT(onServerTypeActivated(int)));

  this->Implementation->UI.ServerType->setCurrentIndex(0);
  this->onServerTypeActivated(0);
  
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
    SIGNAL(serverStarted()),
    this,
    SLOT(onServerStarted()));
}

pqServerBrowser::~pqServerBrowser()
{
  delete this->Implementation;
}

void pqServerBrowser::onServerTypeActivated(int Index)
{
  this->Implementation->UI.HostName->setEnabled(1 == Index);
  this->Implementation->UI.PortNumber->setEnabled(1 == Index);
}

void pqServerBrowser::accept()
{
  pqServerResource server;
  
  switch(this->Implementation->UI.ServerType->currentIndex())
    {
    case 0:
      server.setScheme("builtin");
      break;
      
    case 1:
      server.setScheme("cs");
      server.setHost(this->Implementation->UI.HostName->text());
      server.setPort(this->Implementation->UI.PortNumber->value());
      break;

    default:
      QMessageBox::critical(this, tr("Pick Server:"), tr("Internal error: unknown server type"));
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

void pqServerBrowser::onServerStarted()
{
  if(pqServer* const server = pqApplicationCore::instance()->createServer(
    this->Implementation->Server))
    {
    pqServerResources& resources =
      pqApplicationCore::instance()->serverResources();
//    resources.open(this->Implementation->Server);
    resources.add(this->Implementation->Server);

    emit this->serverConnected(server);
    }

  Superclass::accept();
}
