/*=========================================================================

   Program: ParaView
   Module:    pqManageServersDialog.cxx

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
#include "pqEditServerStartupDialog.h"
#include "pqManageServersDialog.h"
#include "ui_pqManageServersDialog.h"

#include <pqServerResource.h>
#include <pqServerStartups.h>

class pqManageServersDialog::pqImplementation
{
public:
  pqImplementation(pqServerStartups& startups, pqSettings& settings) :
    Startups(startups),
    Settings(settings)
  {
  }

  pqServerStartups& Startups;
  pqSettings& Settings;
  Ui::pqManageServersDialog UI;
};

pqManageServersDialog::pqManageServersDialog(
  pqServerStartups& startups, pqSettings& settings, QWidget* widget_parent) :
    Superclass(widget_parent),
    Implementation(new pqImplementation(startups, settings))
{
  this->Implementation->UI.setupUi(this);
  this->onStartupsChanged();
  this->onSelectionChanged();
  
  connect(&this->Implementation->Startups, SIGNAL(changed()),
    this, SLOT(onStartupsChanged()));
  connect(this->Implementation->UI.addServer, SIGNAL(clicked()),
    this, SLOT(onAddServer()));
  connect(this->Implementation->UI.editServer, SIGNAL(clicked()),
    this, SLOT(onEditServer()));
  connect(this->Implementation->UI.deleteServer, SIGNAL(clicked()),
    this, SLOT(onDeleteServer()));
  connect(this->Implementation->UI.selectAll, SIGNAL(clicked()),
    this, SLOT(onSelectAll()));
  connect(this->Implementation->UI.servers, SIGNAL(itemSelectionChanged()),
    this, SLOT(onSelectionChanged()));
  connect(this->Implementation->UI.servers, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onItemDoubleClicked(QListWidgetItem*)));
}

pqManageServersDialog::~pqManageServersDialog()
{
  delete this->Implementation;
}

void pqManageServersDialog::onStartupsChanged()
{
  this->Implementation->UI.servers->clear();
  
  const pqServerStartups::ServersT servers = this->Implementation->Startups.servers();
  for(int i = 0; i != servers.size(); ++i)
    {
    this->Implementation->UI.servers->addItem(servers[i].toString());
    }
}

void pqManageServersDialog::onAddServer()
{
  pqCreateServerStartupDialog create_server_dialog;
  if(QDialog::Accepted == create_server_dialog.exec())
    {
    pqEditServerStartupDialog edit_server_dialog(
      this->Implementation->Startups, create_server_dialog.getServer());
    if(QDialog::Accepted == edit_server_dialog.exec())
      {
      this->Implementation->Startups.save(this->Implementation->Settings);
      }
    }
}

void pqManageServersDialog::onEditServer()
{
  pqServerStartups::ServersT servers;
  for(int row = 0; row != this->Implementation->UI.servers->count(); ++row)
    {
    QListWidgetItem* const item = this->Implementation->UI.servers->item(row);
    if(this->Implementation->UI.servers->isItemSelected(item))
      {
      pqEditServerStartupDialog dialog(this->Implementation->Startups, item->text());
      if(QDialog::Accepted == dialog.exec())
        {
        this->Implementation->Startups.save(this->Implementation->Settings);
        }
      break;
      }
    }
}

void pqManageServersDialog::onDeleteServer()
{
  pqServerStartups::ServersT servers;
  for(int row = 0; row != this->Implementation->UI.servers->count(); ++row)
    {
    QListWidgetItem* const item = this->Implementation->UI.servers->item(row);
    if(this->Implementation->UI.servers->isItemSelected(item))
      {
      servers.push_back(item->text());
      }
    }
    
  this->Implementation->Startups.deleteStartups(servers);
}

void pqManageServersDialog::onSelectAll()
{
  for(int row = 0; row != this->Implementation->UI.servers->count(); ++row)
    {
    this->Implementation->UI.servers->setItemSelected(
      this->Implementation->UI.servers->item(row), true);
    }
}

void pqManageServersDialog::onSelectionChanged()
{
  const int row_count = this->Implementation->UI.servers->count();
  int selected_count = 0;
  
  for(int row = 0; row != row_count; ++row)
    {
    selected_count += this->Implementation->UI.servers->isItemSelected(
      this->Implementation->UI.servers->item(row));
    }

  this->Implementation->UI.editServer->setEnabled(selected_count == 1);
  this->Implementation->UI.deleteServer->setEnabled(selected_count);
  this->Implementation->UI.selectAll->setEnabled(selected_count != row_count);
}

void pqManageServersDialog::accept()
{
  this->Implementation->Startups.save(this->Implementation->Settings);
  Superclass::accept();
}

void pqManageServersDialog::onItemDoubleClicked(QListWidgetItem* item)
{
  if(item)
    {
    pqEditServerStartupDialog dialog(this->Implementation->Startups, item->text());
    if(QDialog::Accepted == dialog.exec())
      {
      this->Implementation->Startups.save(this->Implementation->Settings);
      }
    }
}
