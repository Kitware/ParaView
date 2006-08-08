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

#include <pqServerResource.h>
#include <pqServerStartupContext.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>

class pqSimpleServerStartup::pqImplementation
{
public:
  pqImplementation(pqSettings& settings, pqServerStartups& startups) :
    Settings(settings),
    Startups(startups),
    StartupContext(0),
    StartupDialog(0)
  {
  }
  
  ~pqImplementation()
  {
    this->reset();
  }

  void reset()
  {
    delete this->StartupContext;
    this->StartupContext = 0;
    
    delete this->StartupDialog;
    this->StartupDialog = 0;
  }

  pqSettings& Settings;
  pqServerStartups& Startups;
  pqServerStartupContext* StartupContext;
  pqServerStartupDialog* StartupDialog;
};

pqSimpleServerStartup::pqSimpleServerStartup(
    pqSettings& settings,
    pqServerStartups& startups,
    QObject* p) :
  Superclass(p),
  Implementation(new pqImplementation(settings, startups))
{
}

pqSimpleServerStartup::~pqSimpleServerStartup()
{
  delete this->Implementation;
}

void pqSimpleServerStartup::startServer(const pqServerResource& server)
{
  // Cleanup old connections ...
  this->Implementation->reset();
  
  // If no startup is required for this server, we're done ...
  pqServerStartups& startups = this->Implementation->Startups;
  if(!startups.startupRequired(server))
    {
    emit this->serverStarted();
    return;
    }

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

  // Try starting the server ...
  if(pqServerStartup* const startup = startups.getStartup(server))
    {
    this->Implementation->StartupContext = new pqServerStartupContext();
    
    this->connect(
      this->Implementation->StartupContext,
      SIGNAL(succeeded()),
      this,
      SLOT(onServerStarted()));
      
    this->connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this,
      SLOT(onServerFailed()));
    
    this->Implementation->StartupDialog = new pqServerStartupDialog(server);
    this->Implementation->StartupDialog->show();
    
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(succeeded()),
      this->Implementation->StartupDialog,
      SLOT(hide()));
      
    QObject::connect(
      this->Implementation->StartupContext,
      SIGNAL(failed()),
      this->Implementation->StartupDialog,
      SLOT(hide()));
    
    startup->execute(server, *this->Implementation->StartupContext);
    }
}

void pqSimpleServerStartup::onServerFailed()
{
  emit this->serverFailed();
}

void pqSimpleServerStartup::onServerStarted()
{
  emit this->serverStarted();
}
