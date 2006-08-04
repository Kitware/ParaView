/*=========================================================================

   Program: ParaView
   Module:    pqEditServerStartupDialog.cxx

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
#include "ui_pqEditServerStartupDialog.h"

#include <pqApplicationCore.h>
#include <pqManualServerStartup.h>
#include <pqServerResource.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>
#include <pqShellServerStartup.h>

#include <QtDebug>

class pqEditServerStartupDialog::pqImplementation
{
public:
  pqImplementation(
    pqServerStartups& startups,
    const pqServerResource& server) :
    Startups(startups),
    Server(server)
  {
  }

  Ui::pqEditServerStartupDialog UI;
  pqServerStartups& Startups;
  const pqServerResource Server;
};

pqEditServerStartupDialog::pqEditServerStartupDialog(
  pqServerStartups& startups,
  const pqServerResource& server,
  QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation(startups, server))
{
  this->Implementation->UI.setupUi(this);
  this->Implementation->UI.message->setText(
    QString("Configure startup for server %1").arg(
      server.schemeHosts().toString()));
    
  if(pqServerStartup* const startup = startups.getStartup(server))
    {
      if(pqShellServerStartup* const shell_startup =
          dynamic_cast<pqShellServerStartup*>(startup))
        {
        this->Implementation->UI.type->setCurrentIndex(0);
        this->Implementation->UI.commandLine->setPlainText(
          shell_startup->CommandLine);
        this->Implementation->UI.delay->setValue(shell_startup->Delay);
        }
      else if(pqManualServerStartup* const manual_startup =
          dynamic_cast<pqManualServerStartup*>(startup))
        {
        this->Implementation->UI.type->setCurrentIndex(1);
        }
    }
  else
    {
    this->Implementation->UI.type->setCurrentIndex(0);
    }
}

pqEditServerStartupDialog::~pqEditServerStartupDialog()
{
  delete this->Implementation;
}

void pqEditServerStartupDialog::accept()
{
  pqServerStartups& startups = this->Implementation->Startups;
  
  switch(this->Implementation->UI.type->currentIndex())
    {
    case 0:
      startups.setShellStartup(
        this->Implementation->Server,
        this->Implementation->UI.commandLine->toPlainText(),
        this->Implementation->UI.delay->value());
      break;
    case 1:
      startups.setManualStartup(
        this->Implementation->Server);
      break;
    default:
      qWarning() << "Unknown server startup type";
      break;
    }
  
  Superclass::accept();
}
