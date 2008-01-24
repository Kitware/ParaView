/*=========================================================================

   Program: ParaView
   Module:    pqPluginDialog.cxx

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

// self
#include "pqPluginDialog.h"

// Qt
#include <QMessageBox>

// SM

// pqCore
#include "pqPluginManager.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqSettings.h"

// pqComponents

pqPluginDialog::pqPluginDialog(pqServer* server, QWidget* p)
  : QDialog(p), Server(server)
{
  this->setupUi(this);

  QString helpText;
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();


  QObject::connect(this->loadRemote, SIGNAL(clicked(bool)),
                   this, SLOT(loadRemotePlugin()));
  QObject::connect(this->loadLocal, SIGNAL(clicked(bool)),
                   this, SLOT(loadLocalPlugin()));
  this->refresh();
  if(!this->Server || !this->Server->isRemote())
    {
    this->remoteGroup->setEnabled(false);
    helpText = "Local plugins are automatically searched for in %1.";
    QStringList serverPaths = pm->pluginPaths(NULL);
    helpText = helpText.arg(serverPaths.join(", "));
    }
  else
    {
    helpText = "Remote plugins are automatically searched for in %1.\n"
               "Local plugins are automatically searched for in %2.";
    QStringList serverPaths = pm->pluginPaths(server);
    helpText = helpText.arg(serverPaths.join(", "));
    QStringList localPaths = pm->pluginPaths(NULL);
    helpText = helpText.arg(localPaths.join(", "));
    }

  this->HelpText->setText(helpText);

  // get remembered plugins
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList local = settings->value("/localPlugins").toStringList();
  QStringList remote = settings->value("/remotePlugins").toStringList();

  this->RecentLocalCombo->addItems(local);
  this->RecentRemoteCombo->addItems(remote);

  QObject::connect(this->RecentLocalCombo, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(loadRecentLocalPlugin(int)));
  QObject::connect(this->RecentRemoteCombo, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(loadRecentRemotePlugin(int)));

}

pqPluginDialog::~pqPluginDialog()
{
}

void pqPluginDialog::loadRemotePlugin()
{
  QString plugin = this->loadPlugin(this->Server);
  if(!plugin.isEmpty())
    {
    this->refreshRemote();
    this->RecentRemoteCombo->addItem(plugin);
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList remote = settings->value("/remotePlugins").toStringList();
    remote.removeAll(plugin);
    remote.insert(0, plugin);
    while(remote.count() > 10)
      {
      remote.removeLast();
      }
    settings->setValue("/remotePlugins", remote);
    }
}

void pqPluginDialog::loadLocalPlugin()
{
  QString plugin = this->loadPlugin(NULL);
  if(!plugin.isEmpty())
    {
    this->refreshLocal();
    this->RecentLocalCombo->addItem(plugin);
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList local = settings->value("/localPlugins").toStringList();
    local.removeAll(plugin);
    local.insert(0, plugin);
    while(local.count() > 10)
      {
      local.removeLast();
      }
    settings->setValue("/localPlugins", local);
    }
}

QString pqPluginDialog::loadPlugin(pqServer* server)
{
  pqFileDialog fd(server, this, "Load Plugin", QString(), 
                  "Plugins (*.so;*.dylib;*.dll;*.sl)\nAll Files (*)");
  QString plugin;
  if(fd.exec() == QDialog::Accepted)
    {
    plugin = fd.getSelectedFiles()[0];
    plugin = this->loadPlugin(server, plugin);
    }
  return plugin;
}

QString pqPluginDialog::loadPlugin(pqServer* server, const QString& plugin)
{
  QString error;
  QString ret = plugin;
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  /* a local plugin may contain both server side code and client code */
  pqPluginManager::LoadStatus result1 = pm->loadPlugin(server, plugin, &error);
  pqPluginManager::LoadStatus result2 = pqPluginManager::LOADED;
  if(!server)
    {
    result2 = pm->loadPlugin(this->Server, plugin, &error);
    }
  
  if(result1 == pqPluginManager::NOTLOADED && 
     result2 == pqPluginManager::NOTLOADED)
    {
    QMessageBox::information(NULL, "Plugin Load Failed", error);
    ret = QString();
    }

  if(result1 != pqPluginManager::LOADED && result2 != pqPluginManager::LOADED)
    {
    ret = QString();
    }
  return ret;
}

void pqPluginDialog::loadRecentRemotePlugin(int idx)
{
  if(idx != 0)
    {
    QString plugin = this->RecentRemoteCombo->itemText(idx);
    this->loadPlugin(this->Server, plugin);
    this->RecentRemoteCombo->setCurrentIndex(0);
    this->refreshRemote();
    }
}

void pqPluginDialog::loadRecentLocalPlugin(int idx)
{
  if(idx != 0)
    {
    QString plugin = this->RecentLocalCombo->itemText(idx);
    this->loadPlugin(NULL, plugin);
    this->RecentLocalCombo->setCurrentIndex(0);
    this->refreshLocal();
    }
}

void pqPluginDialog::refresh()
{
  this->refreshLocal();
  this->refreshRemote();
}

void pqPluginDialog::refreshLocal()
{
  QStringList allplugins;

  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  foreach(QString p, pm->loadedPlugins(NULL))
    {
    allplugins.append(QString("client - %1").arg(p));
    }
  if(!this->Server->isRemote())
    {
    foreach(QString p, pm->loadedPlugins(this->Server))
      {
      allplugins.append(QString("server - %1").arg(p));
      }
    }

  this->localPlugins->clear();
  this->localPlugins->addItems(allplugins);
}

void pqPluginDialog::refreshRemote()
{
  if(this->Server && this->Server->isRemote())
    {
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    QStringList plugins = pm->loadedPlugins(this->Server);
    this->remotePlugins->clear();
    this->remotePlugins->addItems(plugins);
    }
}

