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

