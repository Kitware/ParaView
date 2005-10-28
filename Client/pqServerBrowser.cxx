/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqServer.h"
#include "pqServerBrowser.h"

#include <QMessageBox>

pqServerBrowser::pqServerBrowser(QWidget* Parent) :
  base(Parent)
{
  this->ui.setupUi(this);
  
  this->ui.serverType->addItem(tr("Builtin"));
  this->ui.serverType->addItem(tr("Remote"));

  QObject::connect(this->ui.serverType, SIGNAL(activated(int)), this, SLOT(onServerTypeActivated(int)));

  this->ui.serverType->setCurrentIndex(0);
  this->onServerTypeActivated(0);

  this->setWindowTitle(tr("Pick Server:"));
  
  this->setObjectName("serverBrowser");
  this->ui.serverType->setObjectName("serverBrowser/serverType");
  this->ui.hostName->setObjectName("serverBrowser/hostName");
  this->ui.portNumber->setObjectName("serverBrowser/portNumber");
  this->ui.okButton->setObjectName("serverBrowser/okButton");
  this->ui.cancelButton->setObjectName("serverBrowser/cancelButton");
}

pqServerBrowser::~pqServerBrowser()
{
}

void pqServerBrowser::accept()
{
  pqServer* server = 0;
  switch(ui.serverType->currentIndex())
    {
    case 0:
      server = pqServer::Standalone();
      break;
    case 1:
      server = pqServer::Connect(ui.hostName->text().toAscii().data(), ui.portNumber->value());
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
  delete this;
}

void pqServerBrowser::reject()
{
  base::reject();
  delete this;
}

void pqServerBrowser::onServerTypeActivated(int Index)
{
  this->ui.hostName->setEnabled(1 == Index);
  this->ui.portNumber->setEnabled(1 == Index);
}

