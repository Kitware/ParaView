/*=========================================================================

   Program: ParaView
   Module:    pqServerStartupDialog.cxx

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
#include "pqServerStartupDialog.h"
#include "ui_pqServerStartupDialog.h"

#include <pqServerResource.h>

#include <QCloseEvent>
#include "vtkObjectFactory.h"
class pqServerStartupDialog::pqImplementation
{
public:
  pqImplementation(const pqServerResource& server, bool cancelable) :
    Server(server),
    Cancelable(cancelable)
  {
  }

  Ui::pqServerStartupDialog UI;
  const pqServerResource Server;
  const bool Cancelable;
};

pqServerStartupDialog::pqServerStartupDialog(
  const pqServerResource& server,
  bool cancelable,
  QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation(server, cancelable))
{
  this->Implementation->UI.setupUi(this);
  
  this->Implementation->UI.cancel->setVisible(cancelable);
  
  pqServerResource full_server = server;
  full_server.setPort(server.port(11111));
  full_server.setDataServerPort(server.dataServerPort(11111));
  full_server.setRenderServerPort(server.renderServerPort(22221));
  
  this->Implementation->UI.message->setText(
    QString("Please wait while server %1 starts ...").arg(full_server.toURI()));
   
  // this message is essential for testing to work correctly for reverse
  // connections.
  cout << "Waiting for server..." << endl;
  this->setModal(true);
}

pqServerStartupDialog::~pqServerStartupDialog()
{
  delete this->Implementation;
}

void pqServerStartupDialog::closeEvent(QCloseEvent* e)
{
  e->ignore();
}

void pqServerStartupDialog::reject()
{
  if(this->Implementation->Cancelable)
    {
    Superclass::reject();
    }
}
