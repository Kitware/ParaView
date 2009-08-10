/*=========================================================================

   Program: ParaView
   Module:    pqPluginDialog.cxx

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

// self
#include "pqPluginDialog.h"

// Qt
#include <QHeaderView>
#include <QMessageBox>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

// SM
#include "vtkPVPluginInformation.h"

// pqCore
#include "pqPluginManager.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqSettings.h"

// enum for different columns
enum PluginTreeCol
{
  NameCol         = 0,
  ValueCol        = 1,
};

pqPluginDialog::pqPluginDialog(pqServer* server, QWidget* p)
  : QDialog(p), Server(server)
{
  this->setupUi(this);
  this->setupTreeWidget(this->remotePlugins);
  this->setupTreeWidget(this->localPlugins);
  
  QObject::connect(this->remotePlugins, SIGNAL(itemSelectionChanged()),
    this, SLOT(onRemoteSelectionChanged()), Qt::QueuedConnection);
  QObject::connect(this->localPlugins, SIGNAL(itemSelectionChanged()),
    this, SLOT(onLocalSelectionChanged()), Qt::QueuedConnection);
  
  this->localPlugins->setContextMenuPolicy(Qt::ActionsContextMenu);
  QObject::connect(this->actionReload_Local_Plugin, SIGNAL(triggered()), 
    this, SLOT(onLoadSelectedLocalPlugin()));
  this->localPlugins->addAction(this->actionReload_Local_Plugin);
  this->actionReload_Local_Plugin->setEnabled(0);
  
  this->remotePlugins->setContextMenuPolicy(Qt::ActionsContextMenu);
  QObject::connect(this->actionReload_Remote_Plugin, SIGNAL(triggered()), 
    this, SLOT(onLoadSelectedRemotePlugin()));
  this->remotePlugins->addAction(this->actionReload_Remote_Plugin);
  this->actionReload_Remote_Plugin->setEnabled(0);

  QString helpText;
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();

  QObject::connect(this->loadRemote, SIGNAL(clicked(bool)),
                   this, SLOT(loadRemotePlugin()));
  QObject::connect(this->loadLocal, SIGNAL(clicked(bool)),
                   this, SLOT(loadLocalPlugin()));

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
  
  QObject::connect(pm, SIGNAL(serverManagerExtensionLoaded()),
    this, SLOT(refresh()));
    
  // get remembered plugins
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList local = settings->value("/RecentLocalPlugins").toStringList();
  QStringList remote = settings->value("/RecentRemotePlugins").toStringList();

  this->RecentLocalCombo->addItems(local);
  this->RecentRemoteCombo->addItems(remote);

  QObject::connect(this->RecentLocalCombo, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(loadRecentLocalPlugin(int)));
  QObject::connect(this->RecentRemoteCombo, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(loadRecentRemotePlugin(int)));
  
  this->refresh();
}

pqPluginDialog::~pqPluginDialog()
{
  pqApplicationCore::instance()->getPluginManager()->savePluginSettings(false);
}

void pqPluginDialog::loadRemotePlugin()
{
  QString plugin = this->loadPlugin(this->Server, true);
  if(!plugin.isEmpty())
    {
    this->refresh();
    this->RecentRemoteCombo->addItem(plugin);
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList remote = settings->value("/RecentRemotePlugins").toStringList();
    remote.removeAll(plugin);
    remote.insert(0, plugin);
    while(remote.count() > 10)
      {
      remote.removeLast();
      }
    settings->setValue("/RecentRemotePlugins", remote);
    }
}

void pqPluginDialog::loadLocalPlugin()
  {
  QString plugin = this->loadPlugin(this->Server, false);
  if(!plugin.isEmpty())
    {
    this->refresh();
    this->RecentLocalCombo->addItem(plugin);
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList local = settings->value("/RecentLocalPlugins").toStringList();
    local.removeAll(plugin);
    local.insert(0, plugin);
    while(local.count() > 10)
      {
      local.removeLast();
      }
    settings->setValue("/RecentLocalPlugins", local);
    }
  }

QString pqPluginDialog::loadPlugin(pqServer* server, bool remote)
{
  pqFileDialog fd(server, this, "Load Plugin", QString(), 
                  "Plugins (*.so;*.dylib;*.dll;*.sl)\n"
                  "Client Resource Files (*.bqrc)\n"
                  "Server Manager XML (*.xml)\n"
                  "All Files (*)");
  QString plugin;
  if(fd.exec() == QDialog::Accepted)
    {
    plugin = fd.getSelectedFiles()[0];
    plugin = this->loadPlugin(server, plugin, remote);
    }
  return plugin;
}

QString pqPluginDialog::loadPlugin(pqServer* server, 
  const QString& plugin, bool remote)
{
  QString error;
  QString ret = plugin;
  // now pass it off to the plugin manager to load everything that this 
  // shared library has
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  pqPluginManager::LoadStatus loadresult = pm->loadExtension(server, plugin, &error, remote);
  
  if (loadresult == pqPluginManager::NOTLOADED)
    {
    QMessageBox::information(NULL, "Plugin Load Failed", error);
    ret = QString();
    }

  if (loadresult != pqPluginManager::LOADED)
    {
    ret = QString();
    }
  return ret;
}

void pqPluginDialog::loadRecentRemotePlugin(int idx)
{
  if(idx >0)
    {
    QString plugin = this->RecentRemoteCombo->itemText(idx);
    this->loadPlugin(this->Server, plugin, true);
    this->RecentRemoteCombo->setCurrentIndex(0);
    this->refresh();
    // Move plugin to top of list in settings.
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList remote = settings->value("/RecentRemotePlugins").toStringList();
    remote.removeAll(plugin);
    remote.insert(0, plugin);
    while(remote.count() > 10)
      {
      remote.removeLast();
      }
    settings->setValue("/RecentRemotePlugins", remote);
    }
}

void pqPluginDialog::loadRecentLocalPlugin(int idx)
{
  if(idx > 0)
    {
    QString plugin = this->RecentLocalCombo->itemText(idx);
    this->loadPlugin(this->Server, plugin, false);
    this->RecentLocalCombo->setCurrentIndex(0);
    this->refresh();
    // Move plugin to top of list in settings.
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList local = settings->value("/RecentLocalPlugins").toStringList();
    local.removeAll(plugin);
    local.insert(0, plugin);
    while(local.count() > 10)
      {
      local.removeLast();
      }
    settings->setValue("/RecentLocalPlugins", local);
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
  this->populatePluginTree(this->localPlugins, pm->loadedExtensions(NULL), false);
  this->localPlugins->resizeColumnToContents(ValueCol);  
}

void pqPluginDialog::refreshRemote()
{
  if(this->Server && this->Server->isRemote())
    {
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    this->populatePluginTree(this->remotePlugins, 
      pm->loadedExtensions(this->Server), true);
    this->remotePlugins->resizeColumnToContents(ValueCol);  
    }
}

//----------------------------------------------------------------------------
void pqPluginDialog::setupTreeWidget(QTreeWidget* pluginTree)
{
//  pluginTree->setHeaderItem(0);
  pluginTree->setColumnCount(2);
  pluginTree->header()->setResizeMode(NameCol, QHeaderView::ResizeToContents);
  
  pluginTree->setHeaderLabels(
    QStringList() << tr("Name") << tr("Property"));
        
  QObject::connect(pluginTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(onPluginItemChanged(QTreeWidgetItem*, int))/*, Qt::QueuedConnection*/);
  QObject::connect(pluginTree, SIGNAL(itemExpanded(QTreeWidgetItem*)),
    this, SLOT(resizeColumn(QTreeWidgetItem*))/*, Qt::QueuedConnection*/);
  QObject::connect(pluginTree, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
    this, SLOT(resizeColumn(QTreeWidgetItem*))/*, Qt::QueuedConnection*/);
}

//----------------------------------------------------------------------------
void pqPluginDialog::populatePluginTree(QTreeWidget* pluginTree,
  QList< vtkPVPluginInformation* >& pluginList, bool remote)
{
  pluginTree->blockSignals(true);
  pluginTree->clear();
  foreach(vtkPVPluginInformation* plInfo, pluginList)
    {
    this->createPluginNode(pluginTree, plInfo, remote);
    }
  pluginTree->blockSignals(false);
}

//----------------------------------------------------------------------------
void pqPluginDialog::createPluginNode(
  QTreeWidget* pluginTree, vtkPVPluginInformation* pluginInfo, bool remote)
{
  QTreeWidgetItem* mNode = new QTreeWidgetItem(
    pluginTree, QTreeWidgetItem::UserType);
  
  QVariant vdata;
  vdata.setValue((void*)pluginInfo);
  mNode->setData(NameCol, Qt::UserRole, vdata);
  this->addPluginInfo(mNode, remote);
}

//----------------------------------------------------------------------------
vtkPVPluginInformation* pqPluginDialog::getPluginInfo(
  QTreeWidgetItem* pluginNode)
{
  return (pluginNode && pluginNode->type() == QTreeWidgetItem::UserType) ? 
    static_cast<vtkPVPluginInformation*>(
    pluginNode->data(NameCol,Qt::UserRole).value<void *>()) : NULL;
}

//----------------------------------------------------------------------------
void pqPluginDialog::addPluginInfo(QTreeWidgetItem* pluginNode, bool remote)
{
  vtkPVPluginInformation* plInfo = this->getPluginInfo(pluginNode);
  if(!plInfo)
    {
    return;
    }
    
  Qt::ItemFlags parentFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  pluginNode->setText(NameCol, plInfo->GetPluginName());
  pluginNode->setFlags(parentFlags);
  pluginNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);
  
  this->addInfoNodes(pluginNode, plInfo, remote);  
}

//----------------------------------------------------------------------------
void pqPluginDialog::addInfoNodes(
  QTreeWidgetItem* pluginNode, vtkPVPluginInformation* plInfo, bool remote)
{
  Qt::ItemFlags infoFlags(Qt::ItemIsEnabled);
  
  // set icon hint
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager(); 
  if(pm->isPluginFuntional(plInfo, remote))
    {
    pluginNode->setIcon(ValueCol, QIcon(":/pqWidgets/Icons/PluginGreen.png"));
    }
  else if(!plInfo->GetLoaded() && !plInfo->GetError() )
    {
    pluginNode->setIcon(ValueCol, QIcon(":/pqWidgets/Icons/PluginYellow.png"));
    }
  else
    {
    pluginNode->setIcon(ValueCol, QIcon(":/pqWidgets/Icons/PluginRed.png"));
    }
    
  QStringList infoText;
  //Version
  infoText << tr("Version") <<  tr(plInfo->GetPluginVersion()); 
  QTreeWidgetItem* infoNode = new QTreeWidgetItem(
    pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  
  // Location
  infoText.clear();
  infoText << tr("Location") <<  tr(plInfo->GetFileName()); 
  infoNode = new QTreeWidgetItem(
    pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  infoNode->setToolTip(ValueCol, tr(plInfo->GetFileName()));
  
  // Depended Plugins
  if(plInfo->GetRequiredPlugins())
    {
    infoText.clear();
    infoText << tr("Required Plugins");
    infoText <<  tr(plInfo->GetRequiredPlugins()); 
    infoNode = new QTreeWidgetItem(
      pluginNode, infoText);
    infoNode->setFlags(infoFlags);
    infoNode->setToolTip(ValueCol, tr(plInfo->GetRequiredPlugins()));
    }
    
  // Load status
  infoText.clear();
  infoText << tr("Status");
  infoText <<  this->getStatusText(plInfo);
  infoNode = new QTreeWidgetItem(
    pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  if(plInfo->GetError())
    {
    infoNode->setToolTip(ValueCol, tr(plInfo->GetError()));
    }
  
  // AutoLoad setting
  infoText.clear();
  infoText << tr("Auto Load") <<  tr(""); 
  infoNode = new QTreeWidgetItem(pluginNode, infoText);
  infoNode->setFlags(infoFlags | Qt::ItemIsUserCheckable);
  infoNode->setCheckState(ValueCol, plInfo->GetAutoLoad() ? Qt::Checked : Qt::Unchecked);
}

//----------------------------------------------------------------------------
void pqPluginDialog::onPluginItemChanged(QTreeWidgetItem* item, int col)
{
  if(item && col == ValueCol)
    {
    vtkPVPluginInformation* plInfo = this->getPluginInfo(
      static_cast<QTreeWidgetItem*>(item->parent()));
    if(plInfo)
      {
      pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
      int autoLoad = item->checkState(col)==Qt::Checked ? 1 : 0;
      pm->updatePluginAutoLoadState(plInfo, autoLoad);
      }
    }
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadSelectedPlugins(QList<QTreeWidgetItem*> selItems,
  pqServer* server, bool remote)
{
  for (int i=0;i<selItems.count();i++)
    {
    vtkPVPluginInformation* plInfo = this->getPluginInfo(selItems.value(i));
    if(plInfo && plInfo->GetFileName() && !plInfo->GetLoaded())
      {
      this->loadPlugin(server, QString(plInfo->GetFileName()), remote);
      }     
    }   
}

//----------------------------------------------------------------------------
void pqPluginDialog::onLoadSelectedRemotePlugin()
{
  this->loadSelectedPlugins(this->remotePlugins->selectedItems(), 
    this->Server, true);
  this->refresh();
}
//----------------------------------------------------------------------------
void pqPluginDialog::onLoadSelectedLocalPlugin()
{
  this->loadSelectedPlugins(this->localPlugins->selectedItems(), 
    this->Server, false);
  this->refresh();
}

//----------------------------------------------------------------------------
void pqPluginDialog::onRemoteSelectionChanged()
{
  this->updateContextAction(this->remotePlugins, 
    this->actionReload_Remote_Plugin);
}
//----------------------------------------------------------------------------
void pqPluginDialog::onLocalSelectionChanged()
{
  this->updateContextAction(this->localPlugins, 
    this->actionReload_Local_Plugin);
}
//----------------------------------------------------------------------------
void pqPluginDialog::updateContextAction(
  QTreeWidget* pluginTree, QAction* loadAction)
{
  bool shouldEnable = false;
  if(int num = pluginTree->selectedItems().count())
    {
    for (int i=0;i<num;i++)
      {
      vtkPVPluginInformation* plInfo = this->getPluginInfo(
        pluginTree->selectedItems().value(i));
      if(plInfo && !plInfo->GetLoaded())
        {
        shouldEnable = true;
        break;
        }     
      }   
    }  
  
  loadAction->setEnabled(shouldEnable);
}

//----------------------------------------------------------------------------
void pqPluginDialog::resizeColumn(QTreeWidgetItem* item)
{
  item->treeWidget()->resizeColumnToContents(ValueCol);
}

//----------------------------------------------------------------------------
QString pqPluginDialog::getStatusText(vtkPVPluginInformation* plInfo)
{
  QString text;
  if(plInfo->GetError())
    {
    text = plInfo->GetLoaded() ? QString("Loaded, but ") : QString("Load Error, ") ;
    text.append(plInfo->GetError());
    }
  else
    {
    text = plInfo->GetLoaded() ? "Loaded" : "Not Loaded";
    }
  return text;
}
