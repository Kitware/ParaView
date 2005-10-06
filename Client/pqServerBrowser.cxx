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

pqServerBrowser::pqServerBrowser(QWidget* Parent, const char* const Name) :
  QDialog(Parent, Name)
{
  this->ui.setupUi(this);
  this->setName(Name);
  
  this->ui.serverType->addItem("Standalone");
  this->ui.serverType->addItem("Remote Server");
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
      server = pqServer::Connect(ui.hostName->text().ascii(), ui.portNumber->value());
      break;
    default:
      QMessageBox::critical(this, tr("Pick Server:"), tr("Unknown server type"));
      return;
    }
  
  if(!server)
    {
    QMessageBox::critical(this, tr("Pick Server:"), tr("Error connecting to server"));
    return;
    }

  emit serverConnected(server);

  QDialog::accept();
  delete this;
}

void pqServerBrowser::reject()
{
  QDialog::reject();
  delete this;
}
