/*=========================================================================

   Program: ParaView
   Module:    pqServerStartupBrowser.cxx

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

#include "pqServerStartupBrowser.h"
#include "pqSimpleServerStartup.h"

#include <pqApplicationCore.h>
#include <pqServerResources.h>
#include <pqServerStartup.h>

//////////////////////////////////////////////////////////////////////////////
// pqServerStartupBrowser::pqImplementation

class pqServerStartupBrowser::pqImplementation
{
public:
  pqImplementation() :
    Startup(0),
    ConnectedServer(0)
  {
  }

  pqSimpleServerStartup SimpleServerStartup;
  pqServerStartup* Startup;
  pqServer* ConnectedServer;
};

//////////////////////////////////////////////////////////////////////////////
// pqServerStartupBrowser

pqServerStartupBrowser::pqServerStartupBrowser(
    pqServerStartups& startups,
    QWidget* Parent) :
  Superclass(startups, Parent),
  Implementation(new pqImplementation())
{
  this->setObjectName("ServerStartupBrowser");

  QObject::connect(
    &this->Implementation->SimpleServerStartup,
    SIGNAL(serverCancelled()),
    this,
    SLOT(onServerCancelled()));

  QObject::connect(
    &this->Implementation->SimpleServerStartup,
    SIGNAL(serverFailed()),
    this,
    SLOT(onServerFailed()));

  QObject::connect(
    &this->Implementation->SimpleServerStartup,
    SIGNAL(serverStarted(pqServer*)),
    this,
    SLOT(onServerStarted(pqServer*)));

  // When the dialog is shown, we always want to create a new connection
  // irrespective of whether the selected connection is already connected
  // or not. 
  this->Implementation->SimpleServerStartup.
    setIgnoreConnectIfAlreadyConnected(false);
}

pqServerStartupBrowser::~pqServerStartupBrowser()
{
  delete this->Implementation;
}

pqServer* pqServerStartupBrowser::getConnectedServer()
{
  return this->Implementation->ConnectedServer;
}

void pqServerStartupBrowser::onServerCancelled()
{
  this->reject();
}

void pqServerStartupBrowser::onServerFailed()
{
  this->reject();
}

void pqServerStartupBrowser::onServerStarted(pqServer* server)
{
  this->Implementation->ConnectedServer = server;
  if(server)
    {
    pqApplicationCore::instance()->serverResources().add(
      this->Implementation->Startup->getServer());

    emit this->serverConnected(server);
    }
    
  this->accept();
}

void pqServerStartupBrowser::onServerSelected(pqServerStartup& startup)
{
  this->Implementation->Startup = &startup;
  this->Implementation->SimpleServerStartup.startServer(startup);
}
