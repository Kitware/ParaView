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

// SM

// pqCore
#include "pqPluginManager.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"

// pqComponents

pqPluginDialog::pqPluginDialog(pqServer* server, QWidget* p)
  : QDialog(p), Server(server)
{
  this->setupUi(this);
  QObject::connect(this->loadServer, SIGNAL(clicked(bool)),
                   this, SLOT(loadServerPlugin()));
  QObject::connect(this->loadClient, SIGNAL(clicked(bool)),
                   this, SLOT(loadClientPlugin()));
  this->refresh();
  if(!this->Server)
    {
    this->serverGroup->setEnabled(false);
    }
}

pqPluginDialog::~pqPluginDialog()
{
}

void pqPluginDialog::loadServerPlugin()
{
  this->loadPlugin(this->Server);
  this->refreshServer();
}

void pqPluginDialog::loadClientPlugin()
{
  this->loadPlugin(NULL);
  this->refreshClient();
}

void pqPluginDialog::loadPlugin(pqServer* server)
{
  pqFileDialog fd(server, this, "Load Plugin", QString(), 
                  "Plugins (*.so;*.dylib;*.dll;*.sl); All Files (*)");
  if(fd.exec() == QDialog::Accepted)
    {
    QString plugin = fd.getSelectedFiles()[0];
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    pm->loadPlugin(server, plugin);
    }
}

void pqPluginDialog::refresh()
{
  this->refreshClient();
  this->refreshServer();
}

void pqPluginDialog::refreshClient()
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  QStringList plugins = pm->loadedPlugins(NULL);
  this->clientPlugins->clear();
  this->clientPlugins->addItems(plugins);
}

void pqPluginDialog::refreshServer()
{
  if(this->Server)
    {
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    QStringList plugins = pm->loadedPlugins(this->Server);
    this->serverPlugins->clear();
    this->serverPlugins->addItems(plugins);
    }
}

