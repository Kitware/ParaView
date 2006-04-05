/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include <QMessageBox>

pqServerBrowser::pqServerBrowser(QWidget* Parent) :
  base(Parent)
{
  this->Ui.setupUi(this);
  
  this->Ui.serverType->addItem(tr("Builtin"));
  this->Ui.serverType->addItem(tr("Remote"));

  QObject::connect(this->Ui.serverType, SIGNAL(activated(int)), this, SLOT(onServerTypeActivated(int)));

  this->Ui.serverType->setCurrentIndex(0);
  this->onServerTypeActivated(0);

  this->setWindowTitle(tr("Pick Server:"));
  
  this->setObjectName("serverBrowser");
}

pqServerBrowser::~pqServerBrowser()
{
}

void pqServerBrowser::accept()
{
  pqServer* server = 0;
  switch(this->Ui.serverType->currentIndex())
    {
    case 0:
      server = pqServer::CreateStandalone();
      break;
    case 1:
      server = pqServer::CreateConnection(this->Ui.hostName->text().toAscii().data(), this->Ui.portNumber->value());
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
  this->Ui.hostName->setEnabled(1 == Index);
  this->Ui.portNumber->setEnabled(1 == Index);
}

