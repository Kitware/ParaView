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

#include "pqServer.h"
#include "pqServerBrowser.h"

#include "ui_pqServerBrowser.h"

#include <QMessageBox>

pqServerBrowser::pqServerBrowser(QWidget* Parent) :
  base(Parent),
  Ui(new Ui::pqServerBrowser())
{
  this->Ui->setupUi(this);
  this->setObjectName("ServerBrowser");
  this->setWindowTitle(tr("Pick Server:"));
  
  this->Ui->ServerType->addItem(tr("Builtin"));
  this->Ui->ServerType->addItem(tr("Remote"));

  QObject::connect(this->Ui->ServerType, SIGNAL(activated(int)), this, SLOT(onServerTypeActivated(int)));

  this->Ui->ServerType->setCurrentIndex(0);
  this->onServerTypeActivated(0);
}

pqServerBrowser::~pqServerBrowser()
{
  delete this->Ui;
}

void pqServerBrowser::accept()
{
  pqServer* server = 0;
  switch(this->Ui->ServerType->currentIndex())
    {
    case 0:
      server = pqServer::CreateStandalone();
      break;
    case 1:
      server = pqServer::CreateConnection(this->Ui->HostName->text().toAscii().data(), this->Ui->PortNumber->value());
    case 2:
      // TODO: Add case where the user connects to render server and data server separately.
      // UI will accept host name and port numbers for both data server and render server.
      break;
    default:
      QMessageBox::critical(this, tr("Pick Server:"), tr("Internal error: unknown server type"));
      return;
    }
  
  if(!server)
    {
    QMessageBox::critical(this, tr("Pick Server:"), tr("Error connecting to server"));
    return;
    }

  emit serverConnected(server);

  base::accept();
}

void pqServerBrowser::onServerTypeActivated(int Index)
{
  this->Ui->HostName->setEnabled(1 == Index);
  this->Ui->PortNumber->setEnabled(1 == Index);
}

