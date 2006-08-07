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

#include "pqEditServerStartupDialog.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerStartupDialog.h"
#include "ui_pqServerBrowser.h"

#include <pqApplicationCore.h>
#include <pqServerResources.h>
#include <pqServerStartup.h>
#include <pqServerStartupContext.h>
#include <pqServerStartups.h>

#include <QMessageBox>

class pqServerBrowser::pqImplementation
{
public:
  pqImplementation() :
    StartupContext(0),
    StartupDialog(0)
  {
  }

  ~pqImplementation()
  {
    this->closeStartup();
  }

  void closeStartup()
  {
    delete this->StartupContext;
    delete this->StartupDialog;
    
    this->StartupContext = 0;
    this->StartupDialog = 0;
  }

  Ui::pqServerBrowser UI;
  pqServerResource Server;
  pqServerStartupContext* StartupContext;
  pqServerStartupDialog* StartupDialog;
};

pqServerBrowser::pqServerBrowser(QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);
  this->setObjectName("ServerBrowser");
  this->setWindowTitle(tr("Pick Server:"));
  
  QObject::connect(this->Implementation->UI.ServerType, SIGNAL(activated(int)), this, SLOT(onServerTypeActivated(int)));

  this->Implementation->UI.ServerType->setCurrentIndex(0);
  this->onServerTypeActivated(0);
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

  // If no startup is required for this server, jump to opening the file ...
  pqServerStartups& startups = pqApplicationCore::instance()->serverStartups();
  if(!startups.startupRequired(server))
    {
    this->onServerStarted();
    return;
    }

  // If a startup isn't available for this server, prompt the user ...    
  if(!startups.startupAvailable(server))
    {
    pqEditServerStartupDialog dialog(
      pqApplicationCore::instance()->serverStartups(), server);
    if(QDialog::Rejected == dialog.exec())
      {
      Superclass::accept();
      return;
      }
    startups.save(*pqApplicationCore::instance()->settings());
    }

  // Try starting the server ...
  if(pqServerStartup* const startup = startups.getStartup(server))
    {
    this->Implementation->StartupContext = new pqServerStartupContext();
    this->connect(this->Implementation->StartupContext, SIGNAL(succeeded()), this, SLOT(onServerStarted()));
    this->connect(this->Implementation->StartupContext, SIGNAL(failed()), this, SLOT(onServerFailed()));
    
    this->Implementation->StartupDialog = new pqServerStartupDialog(server);
    this->Implementation->StartupDialog->show();
    QObject::connect(this->Implementation->StartupContext, SIGNAL(succeeded()), this->Implementation->StartupDialog, SLOT(hide()));
    QObject::connect(this->Implementation->StartupContext, SIGNAL(failed()), this->Implementation->StartupDialog, SLOT(hide()));
    
    startup->execute(server, *this->Implementation->StartupContext);
    return;
    }

  Superclass::accept();
}

void pqServerBrowser::onServerStarted()
{
  pqServer* const server = pqApplicationCore::instance()->createServer(this->Implementation->Server);
  if(server)
    {
    pqServerResources& resources = pqApplicationCore::instance()->serverResources();    
    resources.open(this->Implementation->Server);
    resources.add(this->Implementation->Server);

    emit this->serverConnected(server);
    }

  Superclass::accept();
}

void pqServerBrowser::onServerFailed()
{
  Superclass::accept();
}

