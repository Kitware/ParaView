/*=========================================================================

   Program: ParaView
   Module:    pqCreateServerStartupDialog.cxx

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
#include "pqCreateServerStartupDialog.h"
#include "ui_pqCreateServerStartupDialog.h"

#include <pqServerResource.h>

#include <QtDebug>

class pqCreateServerStartupDialog::pqImplementation
{
public:
  pqImplementation()
  {
  }

  Ui::pqCreateServerStartupDialog UI;
  pqServerResource Server;
};

pqCreateServerStartupDialog::pqCreateServerStartupDialog(QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);

  QObject::connect(this->Implementation->UI.type, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onTypeChanged(int)));
  QObject::connect(this->Implementation->UI.host, SIGNAL(textChanged(const QString&)),
    this, SLOT(onHostChanged(const QString&)));
  QObject::connect(this->Implementation->UI.dataServerHost, SIGNAL(textChanged(const QString&)),
    this, SLOT(onHostChanged(const QString&)));
  QObject::connect(this->Implementation->UI.renderServerHost, SIGNAL(textChanged(const QString&)),
    this, SLOT(onHostChanged(const QString&)));
    
  this->updateServer();
}

pqCreateServerStartupDialog::~pqCreateServerStartupDialog()
{
  delete this->Implementation;
}

const pqServerResource pqCreateServerStartupDialog::getServer() const
{
  return this->Implementation->Server;
}

void pqCreateServerStartupDialog::onTypeChanged(int)
{
  this->updateServer();
}

void pqCreateServerStartupDialog::onHostChanged(const QString&)
{
  this->updateServer();
}

void pqCreateServerStartupDialog::updateServer()
{
  switch(this->Implementation->UI.type->currentIndex())
    {
    case 0:
      this->Implementation->Server.setScheme("cs");

      this->Implementation->UI.hostLabel->setVisible(true);
      this->Implementation->UI.host->setVisible(true);
      this->Implementation->UI.dataServerHostLabel->setVisible(false);
      this->Implementation->UI.dataServerHost->setVisible(false);
      this->Implementation->UI.renderServerHostLabel->setVisible(false);
      this->Implementation->UI.renderServerHost->setVisible(false);
      
      break;
      
    case 1:
      this->Implementation->Server.setScheme("csrc");
      
      this->Implementation->UI.hostLabel->setVisible(true);
      this->Implementation->UI.host->setVisible(true);
      this->Implementation->UI.dataServerHostLabel->setVisible(false);
      this->Implementation->UI.dataServerHost->setVisible(false);
      this->Implementation->UI.renderServerHostLabel->setVisible(false);
      this->Implementation->UI.renderServerHost->setVisible(false);

      break;
      
    case 2:
      this->Implementation->Server.setScheme("cdsrs");
      
      this->Implementation->UI.hostLabel->setVisible(false);
      this->Implementation->UI.host->setVisible(false);
      this->Implementation->UI.dataServerHostLabel->setVisible(true);
      this->Implementation->UI.dataServerHost->setVisible(true);
      this->Implementation->UI.renderServerHostLabel->setVisible(true);
      this->Implementation->UI.renderServerHost->setVisible(true);

      break;
      
    case 3:
      this->Implementation->Server.setScheme("cdsrsrc");
      
      this->Implementation->UI.hostLabel->setVisible(false);
      this->Implementation->UI.host->setVisible(false);
      this->Implementation->UI.dataServerHostLabel->setVisible(true);
      this->Implementation->UI.dataServerHost->setVisible(true);
      this->Implementation->UI.renderServerHostLabel->setVisible(true);
      this->Implementation->UI.renderServerHost->setVisible(true);

      break;
    }
    
  this->Implementation->Server.setHost(this->Implementation->UI.host->text());
  this->Implementation->Server.setDataServerHost(this->Implementation->UI.dataServerHost->text());
  this->Implementation->Server.setRenderServerHost(this->Implementation->UI.renderServerHost->text());
  
  this->Implementation->UI.message->setText(QString("Configure new server ... %1").arg(this->Implementation->Server.toString()));
}

void pqCreateServerStartupDialog::accept()
{
  Superclass::accept();
}

