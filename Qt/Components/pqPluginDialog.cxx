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

// pqComponents

pqPluginDialog::pqPluginDialog(pqServer* server, QWidget* p)
  : QDialog(p), Server(server)
{
  this->setupUi(this);
  QObject::connect(this->loadRemote, SIGNAL(clicked(bool)),
                   this, SLOT(loadRemotePlugin()));
  QObject::connect(this->loadLocal, SIGNAL(clicked(bool)),
                   this, SLOT(loadLocalPlugin()));
  this->refresh();
  if(!this->Server || !this->Server->isRemote())
    {
    this->remoteGroup->setEnabled(false);
    }
}

pqPluginDialog::~pqPluginDialog()
{
}

void pqPluginDialog::loadRemotePlugin()
{
  this->loadPlugin(this->Server);
  this->refreshRemote();
}

void pqPluginDialog::loadLocalPlugin()
{
  this->loadPlugin(NULL);
  this->refreshLocal();
}

void pqPluginDialog::loadPlugin(pqServer* server)
{
  pqFileDialog fd(server, this, "Load Plugin", QString(), 
                  "Plugins (*.so;*.dylib;*.dll;*.sl)\nAll Files (*)");
  if(fd.exec() == QDialog::Accepted)
    {
    QString error;
    QString plugin = fd.getSelectedFiles()[0];
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    /* a local plugin may contain both server side code and client code */
    bool result1 = pm->loadPlugin(server, plugin, &error);
    bool result2 = true;
    if(!server)
      {
      result2 = pm->loadPlugin(this->Server, plugin, &error);
      }
    
    if(!result1 && !result2)
      {
      QMessageBox::information(NULL, "Plugin Load Failed", error);
      }
  }
}

void pqPluginDialog::refresh()
{
  this->refreshLocal();
  this->refreshRemote();
}

void pqPluginDialog::refreshLocal()
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  QStringList plugins = pm->loadedPlugins(NULL);
  this->localPlugins->clear();
  this->localPlugins->addItems(plugins);
  if(!this->Server->isRemote())
    {
    plugins = pm->loadedPlugins(this->Server);
    this->localPlugins->addItems(plugins);
    }
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

