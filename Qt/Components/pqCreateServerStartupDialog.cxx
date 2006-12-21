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
};

pqCreateServerStartupDialog::pqCreateServerStartupDialog(
  const pqServerResource& server, QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);
  
  if(server.scheme() == "cs")
    {
    this->Implementation->UI.type->setCurrentIndex(0);
    }
  else if(server.scheme() == "csrc")
    {
    this->Implementation->UI.type->setCurrentIndex(1);
    }
  else if(server.scheme() == "cdsrs")
    {
    this->Implementation->UI.type->setCurrentIndex(2);
    }
  else if(server.scheme() == "cdsrsrc")
    {
    this->Implementation->UI.type->setCurrentIndex(3);
    }
    
  this->Implementation->UI.host->setText(server.host());
  this->Implementation->UI.dataServerHost->setText(server.dataServerHost());
  this->Implementation->UI.renderServerHost->setText(server.renderServerHost());

  QObject::connect(
    this->Implementation->UI.type,
    SIGNAL(currentIndexChanged(int)),
    this,
    SLOT(updateServerType()));
    
  QObject::connect(
    this->Implementation->UI.type,
    SIGNAL(currentIndexChanged(int)),
    this,
    SLOT(updateConnectButton()));

  QObject::connect(
    this->Implementation->UI.name,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(updateConnectButton()));

  QObject::connect(
    this->Implementation->UI.host,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(updateConnectButton()));

  QObject::connect(
    this->Implementation->UI.dataServerHost,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(updateConnectButton()));

  QObject::connect(
    this->Implementation->UI.renderServerHost,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(updateConnectButton()));

  this->updateServerType();
  this->updateConnectButton();
}

pqCreateServerStartupDialog::~pqCreateServerStartupDialog()
{
  delete this->Implementation;
}

const QString pqCreateServerStartupDialog::getName()
{
  return this->Implementation->UI.name->text();
}

const pqServerResource pqCreateServerStartupDialog::getServer()
{
  pqServerResource server;
  
  switch(this->Implementation->UI.type->currentIndex())
    {
    case 0:
      server.setScheme("cs");
      server.setHost(this->Implementation->UI.host->text());
      break;
      
    case 1:
      server.setScheme("csrc");
      server.setHost(this->Implementation->UI.host->text());
      break;
      
    case 2:
      server.setScheme("cdsrs");
      server.setDataServerHost(this->Implementation->UI.dataServerHost->text());
      server.setRenderServerHost(this->Implementation->UI.renderServerHost->text());
      break;
      
    case 3:
      server.setScheme("cdsrsrc");
      server.setDataServerHost(this->Implementation->UI.dataServerHost->text());
      server.setRenderServerHost(this->Implementation->UI.renderServerHost->text());
      break;
    }
  
  return server;
}

void pqCreateServerStartupDialog::updateServerType()
{
  switch(this->Implementation->UI.type->currentIndex())
    {
    case 0:
      this->Implementation->UI.hostLabel->setVisible(true);
      this->Implementation->UI.host->setVisible(true);
      this->Implementation->UI.dataServerHostLabel->setVisible(false);
      this->Implementation->UI.dataServerHost->setVisible(false);
      this->Implementation->UI.renderServerHostLabel->setVisible(false);
      this->Implementation->UI.renderServerHost->setVisible(false);
      break;
      
    case 1:
      this->Implementation->UI.hostLabel->setVisible(true);
      this->Implementation->UI.host->setVisible(true);
      this->Implementation->UI.dataServerHostLabel->setVisible(false);
      this->Implementation->UI.dataServerHost->setVisible(false);
      this->Implementation->UI.renderServerHostLabel->setVisible(false);
      this->Implementation->UI.renderServerHost->setVisible(false);
      break;
      
    case 2:
      this->Implementation->UI.hostLabel->setVisible(false);
      this->Implementation->UI.host->setVisible(false);
      this->Implementation->UI.dataServerHostLabel->setVisible(true);
      this->Implementation->UI.dataServerHost->setVisible(true);
      this->Implementation->UI.renderServerHostLabel->setVisible(true);
      this->Implementation->UI.renderServerHost->setVisible(true);
      break;
      
    case 3:
      this->Implementation->UI.hostLabel->setVisible(false);
      this->Implementation->UI.host->setVisible(false);
      this->Implementation->UI.dataServerHostLabel->setVisible(true);
      this->Implementation->UI.dataServerHost->setVisible(true);
      this->Implementation->UI.renderServerHostLabel->setVisible(true);
      this->Implementation->UI.renderServerHost->setVisible(true);
      break;
    }
}

void pqCreateServerStartupDialog::updateConnectButton()
{
  switch(this->Implementation->UI.type->currentIndex())
    {
    case 0:
    case 1:
      this->Implementation->UI.okButton->setEnabled(
        !this->Implementation->UI.name->text().isEmpty() &&
        !this->Implementation->UI.host->text().isEmpty());
      break;
      
    case 2:
    case 3:
      this->Implementation->UI.okButton->setEnabled(
        !this->Implementation->UI.name->text().isEmpty() &&
        !this->Implementation->UI.dataServerHost->text().isEmpty() &&
        !this->Implementation->UI.renderServerHost->text().isEmpty());
      break;
    }
}

void pqCreateServerStartupDialog::accept()
{
  Superclass::accept();
}

