/*=========================================================================

   Program: ParaView
   Module:    pqSimpleServerStartup.cxx

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
#include "pqServerStartupDialog.h"
#include "pqSimpleServerStartup.h"

#include <pqApplicationCore.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqServerResource.h>
#include <pqServerStartupContext.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>

#include <vtkProcessModule.h>
#include <vtkProcessModuleConnectionManager.h>

#include <QTimer>
#include <QtDebug>

class pqSimpleServerStartup::pqImplementation
{
public:
  pqImplementation(pqSettings& settings, pqServerStartups& startups) :
    Settings(settings),
    Startups(startups),
    StartupContext(0),
    StartupDialog(0),
    ServerReverseConnection(0)
  {
  }
  
  ~pqImplementation()
  {
    this->reset();
  }

  void reset()
  {
    this->Server = pqServerResource();
  
    delete this->StartupContext;
    this->StartupContext = 0;
    
    delete this->StartupDialog;
    this->StartupDialog = 0;
    
    this->ServerReverseConnection = 0;
    
    this->ReverseTimer.stop();
  }

  pqSettings& Settings;
  pqServerStartups& Startups;
  pqServerResource Server;
  pqServerStartupContext* StartupContext;
  pqServerStartupDialog* StartupDialog;
  QTimer ReverseTimer;
  int ServerReverseConnection;
  
};

pqSimpleServerStartup::pqSimpleServerStartup(
    pqSettings& settings,
    pqServerStartups& startups,
    QObject* p) :
  Superclass(p),
  Implementation(new pqImplementation(settings, startups))
{
  this->Implementation->ReverseTimer.setInterval(10);
  
  QObject::connect(
    &this->Implementation->ReverseTimer,
    SIGNAL(timeout()),
    this,
    SLOT(monitorReverseConnections()));
}

pqSimpleServerStartup::~pqSimpleServerStartup()
{
  delete this->Implementation;
}

void pqSimpleServerStartup::startServer(const pqServerResource& resource)
{
  // Cleanup old connections ...
  this->Implementation->reset();
  
  // Get the actual server to be started ...
  const pqServerResource server = resource.scheme() == "session"
    ? resource.sessionServer().schemeHostsPorts()
    : resource.schemeHostsPorts();
  
  this->Implementation->Server = server;
  
  // If no startup is required for this server, jump to make the connection ...
  pqServerStartups& startups = this->Implementation->Startups;
  if(startups.startupRequired(server))
    {
    // If a startup isn't available for this server, prompt the user ...    
    if(!startups.startupAvailable(server))
      {
      pqEditServerStartupDialog dialog(startups, server);
      if(QDialog::Rejected == dialog.exec())
        {
        emit this->serverCancelled();
        return;
        }
        
      startups.save(this->Implementation->Settings);
      }
    }

  // Branch based on the connection type - builtin, forward, or reverse ...
  if(server.scheme() == "builtin")
    {
    this->startBuiltinConnection();
    }
  else if(server.scheme() == "cs" || server.scheme() == "cdsrs")
    {
    this->startForwardConnection();
    }
  else if(server.scheme() == "csrc" || server.scheme() == "cdsrsrc")
    {
    this->startReverseConnection();
    }
  else
    {
    qCritical() << "Unknown server scheme: " << server.scheme();
    emit this->serverFailed();
    }
}

void pqSimpleServerStartup::startBuiltinConnection()
{
  if(pqServer* const server = pqApplicationCore::instance()->createServer(
    this->Implementation->Server))
    {
    emit this->serverStarted(server);
    }
  else
    {
    emit this->serverFailed();
    }
}

void pqSimpleServerStartup::startForwardConnection()
{
  if(pqServer* const existing_server =
    pqApplicationCore::instance()->getServerManagerModel()->getServer(
      this->Implementation->Server))
    {
    emit this->serverStarted(existing_server);
    }

  if(pqServerStartup* const startup =
    this->Implementation->Startups.getStartup(
      this->Implementation->Server))
    {
    this->Implementation->StartupContext = new pqServerStartupContext();
    
    this->Implementation->StartupDialog =
      new pqServerStartupDialog(this->Implementation->Server);
    this->Implementation->StartupDialog->show();
    
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(succeeded()),
      this,
      SLOT(forwardConnectServer()));
      
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(succeeded()),
      this->Implementation->StartupDialog,
      SLOT(hide()));
      
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this,
      SIGNAL(serverFailed()));
    
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this->Implementation->StartupDialog,
      SLOT(hide()));
    
    startup->execute(
      this->Implementation->Server,
      *this->Implementation->StartupContext);
    }
}

void pqSimpleServerStartup::forwardConnectServer()
{
  if(pqServer* const server = pqApplicationCore::instance()->createServer(
    this->Implementation->Server))
    {
    emit this->serverStarted(server);
    }
  else
    {
    emit this->serverFailed();
    }
}

void pqSimpleServerStartup::startReverseConnection()
{
  if(pqServer* const existing_server =
    pqApplicationCore::instance()->getServerManagerModel()->getServer(
      this->Implementation->Server))
    {
    emit this->serverStarted(existing_server);
    }

  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(reverseConnection(pqServer*)));
  
  if(this->Implementation->Server.scheme() == "csrc")
    {
    process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.port(11111));
    }
  else if(this->Implementation->Server.scheme() == "cdsrsrc")
    {
    int dsid, rsid;
    process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.dataServerPort(11111),
      this->Implementation->Server.renderServerPort(22221),
      dsid,
      rsid);
    }
    
  if(pqServerStartup* const startup =
    this->Implementation->Startups.getStartup(
      this->Implementation->Server))
    {
    this->Implementation->StartupContext = new pqServerStartupContext();

    this->Implementation->StartupDialog =
      new pqServerStartupDialog(this->Implementation->Server);
    this->Implementation->StartupDialog->show();

    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(succeeded()),
      &this->Implementation->ReverseTimer,
      SLOT(start()));
      
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this,
      SIGNAL(serverFailed()));
    
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this->Implementation->StartupDialog,
      SLOT(hide()));
  
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      &this->Implementation->ReverseTimer,
      SLOT(stop()));
  
    startup->execute(
      this->Implementation->Server,
      *this->Implementation->StartupContext);
    }
}

void pqSimpleServerStartup::monitorReverseConnections()
{
  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  process_module->MonitorConnections(10);
}

void pqSimpleServerStartup::reverseConnection(pqServer* server)
{
  QObject::disconnect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(reverseConnection(pqServer*)));

  server->setResource(this->Implementation->Server);

  this->Implementation->ReverseTimer.stop();
  this->Implementation->StartupDialog->hide();
  emit this->serverStarted(server);
}
