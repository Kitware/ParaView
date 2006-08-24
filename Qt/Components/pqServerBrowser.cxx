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

#include "pqCreateServerStartupDialog.h"
#include "pqEditServerStartupDialog.h"
#include "pqServerBrowser.h"
#include "pqSimpleServerStartup.h"
#include "ui_pqServerBrowser.h"

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqServerResources.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>

#include <QFile>
#include <QtDebug>

//////////////////////////////////////////////////////////////////////////////
// pqServerBrowser::pqImplementation

class pqServerBrowser::pqImplementation
{
public:
  pqImplementation(pqServerStartups& startups, pqSettings& settings) :
    Startups(startups),
    Settings(settings),
    SelectedServer(0)
  {
  }

  Ui::pqServerBrowser UI;
  pqServerStartups& Startups;
  pqSettings& Settings;
  pqServerStartup* SelectedServer;
};

//////////////////////////////////////////////////////////////////////////////
// pqServerBrowser

pqServerBrowser::pqServerBrowser(
    pqServerStartups& startups,
    pqSettings& settings,
    QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation(startups, settings))
{
  this->Implementation->UI.setupUi(this);
  this->setObjectName("ServerBrowser");
  
  this->onStartupsChanged();
  this->onCurrentItemChanged(0, 0);

  QObject::connect(
    &this->Implementation->Startups,
    SIGNAL(changed()),
    this,
    SLOT(onStartupsChanged()));
  QObject::connect(
    this->Implementation->UI.startups,
    SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
    this,
    SLOT(onCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
  QObject::connect(
    this->Implementation->UI.startups,
    SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this,
    SLOT(onItemDoubleClicked(QListWidgetItem*)));
  
  QObject::connect(this->Implementation->UI.addServer, SIGNAL(clicked()),
    this, SLOT(onAddServer()));
  QObject::connect(this->Implementation->UI.editServer, SIGNAL(clicked()),
    this, SLOT(onEditServer()));
  QObject::connect(this->Implementation->UI.deleteServer, SIGNAL(clicked()),
    this, SLOT(onDeleteServer()));
  QObject::connect(this->Implementation->UI.save, SIGNAL(clicked()),
    this, SLOT(onSave()));
  QObject::connect(this->Implementation->UI.load, SIGNAL(clicked()),
    this, SLOT(onLoad()));
    
  QObject::connect(this->Implementation->UI.connect, SIGNAL(clicked()),
    this, SLOT(onConnect()));
  QObject::connect(this->Implementation->UI.close, SIGNAL(clicked()),
    this, SLOT(close()));
}

pqServerBrowser::~pqServerBrowser()
{
  delete this->Implementation;
}

void pqServerBrowser::setMessage(const QString& message)
{
  this->Implementation->UI.message->setText(message);
}

pqServerStartup* pqServerBrowser::getSelectedServer()
{
  return this->Implementation->SelectedServer;
}

void pqServerBrowser::onStartupsChanged()
{
  this->Implementation->UI.startups->clear();
  
  const pqServerStartups::StartupsT startups = this->Implementation->Startups.getStartups();
  for(int i = 0; i != startups.size(); ++i)
    {
    this->Implementation->UI.startups->addItem(startups[i]);
    }
}

void pqServerBrowser::onCurrentItemChanged(QListWidgetItem* current, QListWidgetItem*)
{
  int editable_count = 0;
  int deletable_count = 0;

  if(current)
    {
    editable_count += 1;
    
    pqServerStartup* const startup = this->Implementation->Startups.getStartup(current->text());
    if(startup && startup->getOwner() == "user")
      {
      deletable_count += 1;
      }
    }

  this->Implementation->UI.editServer->setEnabled(editable_count == 1);
  this->Implementation->UI.deleteServer->setEnabled(deletable_count == 1);
  this->Implementation->UI.connect->setEnabled(current);
}

void pqServerBrowser::onItemDoubleClicked(QListWidgetItem* item)
{
  if(item)
    {
    if(pqServerStartup* const startup =
      this->Implementation->Startups.getStartup(item->text()))
      {
      this->emitServerSelected(*startup);
      }
    }
}

void pqServerBrowser::onAddServer()
{
  pqCreateServerStartupDialog create_server_dialog;
  if(QDialog::Accepted == create_server_dialog.exec())
    {
    pqEditServerStartupDialog edit_server_dialog(
      this->Implementation->Startups,
      create_server_dialog.getName(),
      create_server_dialog.getServer());
    if(QDialog::Accepted == edit_server_dialog.exec())
      {
      this->Implementation->Startups.save(this->Implementation->Settings);
      }
    }
}

void pqServerBrowser::onEditServer()
{
  for(int row = 0; row != this->Implementation->UI.startups->count(); ++row)
    {
    QListWidgetItem* const item = this->Implementation->UI.startups->item(row);
    if(this->Implementation->UI.startups->isItemSelected(item))
      {
      if(pqServerStartup* const startup = this->Implementation->Startups.getStartup(item->text()))
        {
        pqEditServerStartupDialog dialog(
          this->Implementation->Startups,
          startup->getName(),
          startup->getServer());
        if(QDialog::Accepted == dialog.exec())
          {
          this->Implementation->Startups.save(this->Implementation->Settings);
          }
        }
      }
    }
}

void pqServerBrowser::onDeleteServer()
{
  pqServerStartups::StartupsT startups;
  for(int row = 0; row != this->Implementation->UI.startups->count(); ++row)
    {
    QListWidgetItem* const item = this->Implementation->UI.startups->item(row);
    if(this->Implementation->UI.startups->isItemSelected(item))
      {
      startups.push_back(item->text());
      }
    }
    
  this->Implementation->Startups.deleteStartups(startups);
}

void pqServerBrowser::onSave()
{
  QString filters;
  filters += "ParaView server configuration file (*.pvsc)";
  filters += ";;All files (*)";

  pqFileDialog* const dialog  = new pqFileDialog(new pqLocalFileDialogModel(),
      this, tr("Save Server Configuration File:"), QString(), filters);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setObjectName("SaveServerConfigurationDialog");
  dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(dialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onSave(const QStringList&)));
  dialog->show();
}

void pqServerBrowser::onSave(const QStringList& files)
{
  QDomDocument xml;
  this->Implementation->Startups.save(xml);

  for(int i = 0; i != files.size(); ++i)
    {
    QFile file(files[i]);
    if(!file.open(QIODevice::WriteOnly))
      {
      qCritical() << "Error opening " << files[i] << "for writing";
      }
    file.write(xml.toByteArray());
    }
}

void pqServerBrowser::onLoad()
{
  QString filters;
  filters += "ParaView server configuration file (*.pvsc)";
  filters += ";;All files (*)";

  pqFileDialog* const dialog  = new pqFileDialog(new pqLocalFileDialogModel(),
      this, tr("Load Server Configuration File:"), QString(), filters);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setObjectName("LoadServerConfigurationDialog");
  dialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(dialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onLoad(const QStringList&)));
  dialog->show();
}

void pqServerBrowser::onLoad(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QFile file(files[i]);
    QDomDocument xml;
    if(xml.setContent(&file, false))
      {
      this->Implementation->Startups.load(xml);
      }
    else
      {
      qCritical() << "Error parsing " << files[i] << ": not a valid XML document";
      }
    }
}

void pqServerBrowser::onConnect()
{
  if(this->Implementation->UI.startups->currentItem())
    {
    if(pqServerStartup* const startup = this->Implementation->Startups.getStartup(
      this->Implementation->UI.startups->currentItem()->text()))
      {
      this->emitServerSelected(*startup);
      }
    }
}

void pqServerBrowser::onClose()
{
  this->reject();
}

void pqServerBrowser::emitServerSelected(pqServerStartup& startup)
{
  this->Implementation->SelectedServer = &startup;
  emit this->serverSelected(startup);
  this->onServerSelected(startup);
}

void pqServerBrowser::onServerSelected(pqServerStartup& /*startup*/)
{
  this->accept();
}
