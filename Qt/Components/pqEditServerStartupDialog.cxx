/*=========================================================================

   Program: ParaView
   Module:    pqEditServerStartupDialog.cxx

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
#include "pqEditServerStartupDialog.h"
#include "ui_pqEditServerStartupDialog.h"

#include <pqApplicationCore.h>
#include <pqManualServerStartup.h>
#include <pqServerResource.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>
#include <pqCommandServerStartup.h>

#include <QtDebug>

/////////////////////////////////////////////////////////////////////////
// pqEditServerStartupDialog::pqImplementation

class pqEditServerStartupDialog::pqImplementation
{
public:
  pqImplementation(
    pqServerStartups& startups,
    const QString& name,
    const pqServerResource& server) :
    Startups(startups),
    Name(name),
    Server(server)
  {
  }

  Ui::pqEditServerStartupDialog UI;
  pqServerStartups& Startups;
  const QString Name;
  const pqServerResource Server;
};

/////////////////////////////////////////////////////////////////////////
// pqEditServerStartupDialog

pqEditServerStartupDialog::pqEditServerStartupDialog(
  pqServerStartups& startups,
  const QString& name,
  const pqServerResource& server,
  QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation(startups, name, server))
{
  this->Implementation->UI.setupUi(this);
  
  this->Implementation->UI.message->setText(
    QString(tr("Configure %1 (%2)")).arg(name).arg(
      server.schemeHosts().toURI()));
  this->Implementation->UI.secondaryMessage->setText(
    tr("Please configure the startup procedure to be used when connecting to this server:"));
  this->Implementation->UI.type->setEnabled(true);
  this->Implementation->UI.commandLine->setEnabled(true);
  this->Implementation->UI.delay->setEnabled(true);
  
  if(pqServerStartup* const startup = startups.getStartup(name))
    {
      if(!startup->shouldSave())
        {
        this->Implementation->UI.message->setText(
          QString(tr("%1 (%2) configuration")).arg(name).arg(
            server.schemeHosts().toURI()));
        this->Implementation->UI.secondaryMessage->setText(
          tr("This server was configured by site administrators and cannot be modified."));
          
        this->Implementation->UI.type->setEnabled(false);
        this->Implementation->UI.commandLine->setEnabled(false);
        this->Implementation->UI.delay->setEnabled(false);
        }
    
      if(pqCommandServerStartup* const command_startup =
          qobject_cast<pqCommandServerStartup*>(startup))
        {
        this->Implementation->UI.type->setCurrentIndex(0);
        this->Implementation->UI.stackedWidget->setCurrentIndex(0);

        this->Implementation->UI.commandLine->setPlainText(
          command_startup->getExecutable() + " " + command_startup->getArguments().join(" "));
        this->Implementation->UI.delay->setValue(command_startup->getDelay());
        }
      else if(qobject_cast<pqManualServerStartup*>(startup))
        {
        this->Implementation->UI.type->setCurrentIndex(1);
        this->Implementation->UI.stackedWidget->setCurrentIndex(1);
        }
    }
  else
    {
    this->Implementation->UI.type->setCurrentIndex(0);
    this->Implementation->UI.stackedWidget->setCurrentIndex(0);
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
      {
      QStringList arguments;
      QString command_line = this->Implementation->UI.commandLine->toPlainText().simplified();
      while(command_line.size())
        {
        for(int i = 0; i != command_line.size(); ++i)
          {
          if(command_line.at(i).isSpace() && command_line[0] != '\"')
            {
            arguments.push_back(command_line.left(i));
            command_line.remove(0, i+1);
            break;
            }
            
          if(i && command_line[0] == '\"' && command_line[i] == '\"')
            {
            arguments.push_back(command_line.mid(1, i-1));
            command_line.remove(0, i+2);
            break;
            }
            
          if(i+1 == command_line.size())
            {
            arguments.push_back(command_line);
            command_line.clear();
            break;
            }
          }
        }

      QString executable;      
      if(arguments.size())
        {
        executable = arguments[0];
        arguments.erase(arguments.begin());
        }
        
      startups.setCommandStartup(
        this->Implementation->Name,
        this->Implementation->Server,
        executable,
        0,
        this->Implementation->UI.delay->value(),
        arguments);
      }
      break;
    case 1:
      startups.setManualStartup(
        this->Implementation->Name,
        this->Implementation->Server);
      break;
    default:
      qWarning() << "Unknown server startup type";
      break;
    }
  
  Superclass::accept();
}
