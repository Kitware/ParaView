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
#include "ui_pqPluginDialog.h"

// Qt
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>

// SM
#include "vtkPVPluginsInformation.h"

// pqCore
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPluginManager.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkNew.h"
#include "vtkPVPythonInformation.h"
#include "vtkSMSession.h"

// enum for different columns
enum PluginTreeCol
{
  NameCol = 0,
  ValueCol = 1,
};

//----------------------------------------------------------------------------
pqPluginDialog::pqPluginDialog(pqServer* server, QWidget* p)
  : Superclass(p)
  , Ui(new Ui::pqPluginDialog())
  , Server(server)
{
  this->Ui->setupUi(this);
  this->setupTreeWidget(this->Ui->remotePlugins);
  this->setupTreeWidget(this->Ui->localPlugins);

  QObject::connect(this->Ui->remotePlugins, SIGNAL(itemSelectionChanged()), this,
    SLOT(onRemoteSelectionChanged()), Qt::QueuedConnection);
  QObject::connect(this->Ui->localPlugins, SIGNAL(itemSelectionChanged()), this,
    SLOT(onLocalSelectionChanged()), Qt::QueuedConnection);

  QString helpText;
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();

  QObject::connect(this->Ui->loadRemote, SIGNAL(clicked(bool)), this, SLOT(loadRemotePlugin()));
  QObject::connect(this->Ui->loadLocal, SIGNAL(clicked(bool)), this, SLOT(loadLocalPlugin()));

  if (!this->Server || !this->Server->isRemote())
  {
    this->Ui->remoteGroup->setEnabled(false);
    helpText = "Local plugins are automatically searched for in %1.";
    QStringList serverPaths = pm->pluginPaths(NULL, false);
    helpText = helpText.arg(serverPaths.join(", "));
  }
  else
  {
    helpText = "Remote plugins are automatically searched for in %1.\n"
               "Local plugins are automatically searched for in %2.";
    QStringList serverPaths = pm->pluginPaths(this->Server, true);
    helpText = helpText.arg(serverPaths.join(", "));
    QStringList localPaths = pm->pluginPaths(this->Server, false);
    helpText = helpText.arg(localPaths.join(", "));
  }

  this->Ui->HelpText->setText(helpText);

  QObject::connect(pm, SIGNAL(pluginsUpdated()), this, SLOT(onRefresh()));
  // QObject::connect(pm, SIGNAL(pluginInfoUpdated()),
  //  this, SLOT(refresh()));

  QObject::connect(
    this->Ui->loadSelected_Remote, SIGNAL(clicked(bool)), this, SLOT(onLoadSelectedRemotePlugin()));
  QObject::connect(
    this->Ui->loadSelected_Local, SIGNAL(clicked(bool)), this, SLOT(onLoadSelectedLocalPlugin()));
  QObject::connect(
    this->Ui->removeRemote, SIGNAL(clicked(bool)), this, SLOT(onRemoveSelectedRemotePlugin()));
  QObject::connect(
    this->Ui->removeLocal, SIGNAL(clicked(bool)), this, SLOT(onRemoveSelectedLocalPlugin()));

  this->LoadingMultiplePlugins = false;
  this->refresh();
}

//----------------------------------------------------------------------------
pqPluginDialog::~pqPluginDialog()
{
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadRemotePlugin()
{
  this->loadPlugin(this->Server, true);
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadLocalPlugin()
{
  this->loadPlugin(this->Server, false);
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadPlugin(pqServer* server, bool remote)
{
  std::map<QString, QStringList> exts;
  exts[tr("Qt binary resource files")] << "*.bqrc";
  exts[tr("XML plugins")] << "*.xml";

#if defined(_WIN32) && !defined(__CYGWIN__)
  exts[tr("Binary plugins")] << "*.dll";
#else
  // starting with ParaView 5.7, we are building .so's even on macOS
  // since they are built as "add_library(.. MODULE)" which by default generates
  // `.so`s which seems to be the convention.
  exts[tr("Binary plugins")] << "*.so";
#endif

  vtkNew<vtkPVPythonInformation> pinfo;
  if (remote && server != nullptr)
  {
    server->session()->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, pinfo, 0);
  }
  else
  {
    pinfo->CopyFromObject(nullptr);
  }

  if (pinfo->GetPythonSupport())
  {
    exts["Python plugins"] << "*.py";
  }

  QStringList supportsExts;
  for (const auto apair : exts)
  {
    supportsExts.append(apair.second);
  }
  supportsExts.sort(Qt::CaseInsensitive);

  QString filterString;
  QTextStream stream(&filterString, QIODevice::WriteOnly);

  stream << tr("Supported plugins") << " (" << supportsExts.join(" ") << ");;";
  for (const auto apair : exts)
  {
    stream << apair.first << " (" << apair.second.join(" ") << ");;";
  }
  stream << "All files (*)";

  pqFileDialog fd(remote ? server : NULL, this, "Load Plugin", QString(), filterString);
  if (fd.exec() == QDialog::Accepted)
  {
    QString plugin = fd.getSelectedFiles()[0];
    this->loadPlugin(server, plugin, remote);
  }
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadPlugin(pqServer* server, const QString& plugin, bool remote)
{
  QString error;
  // now pass it off to the plugin manager to load everything that this
  // shared library has
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  pm->loadExtension(server, plugin, &error, remote);
}

//----------------------------------------------------------------------------
void pqPluginDialog::removePlugin(pqServer*, const QString& plugin, bool remote)
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  pm->hidePlugin(plugin, remote);
}

//----------------------------------------------------------------------------
void pqPluginDialog::onRefresh()
{
  if (!this->LoadingMultiplePlugins)
  {
    this->refresh();
  }
}

//----------------------------------------------------------------------------
void pqPluginDialog::refresh()
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  pm->verifyPlugins(this->Server);

  this->refreshLocal();
  this->refreshRemote();
}

//----------------------------------------------------------------------------
void pqPluginDialog::refreshLocal()
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  vtkPVPluginsInformation* extensions = pm->loadedExtensions(this->Server, false);
  this->populatePluginTree(this->Ui->localPlugins, extensions, false);
  this->Ui->localPlugins->resizeColumnToContents(ValueCol);
}

//----------------------------------------------------------------------------
void pqPluginDialog::refreshRemote()
{
  if (this->Server && this->Server->isRemote())
  {
    pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
    vtkPVPluginsInformation* extensions = pm->loadedExtensions(this->Server, true);
    this->populatePluginTree(this->Ui->remotePlugins, extensions, true);
    this->Ui->remotePlugins->resizeColumnToContents(ValueCol);
  }
}

//----------------------------------------------------------------------------
void pqPluginDialog::setupTreeWidget(QTreeWidget* pluginTree)
{
  pluginTree->setColumnCount(2);
#if QT_VERSION >= 0x050000
  pluginTree->header()->setSectionResizeMode(NameCol, QHeaderView::ResizeToContents);
  pluginTree->header()->setSectionResizeMode(ValueCol, QHeaderView::Custom);
#else
  pluginTree->header()->setResizeMode(NameCol, QHeaderView::ResizeToContents);
  pluginTree->header()->setResizeMode(ValueCol, QHeaderView::Custom);
#endif

  pluginTree->setHeaderLabels(QStringList() << tr("Name") << tr("Property"));

  pluginTree->setSortingEnabled(true);
  pluginTree->sortByColumn(-1, Qt::AscendingOrder);

  QObject::connect(pluginTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this,
    SLOT(onPluginItemChanged(QTreeWidgetItem*, int)) /*, Qt::QueuedConnection*/);
  QObject::connect(pluginTree, SIGNAL(itemExpanded(QTreeWidgetItem*)), this,
    SLOT(resizeColumn(QTreeWidgetItem*)) /*, Qt::QueuedConnection*/);
  QObject::connect(pluginTree, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this,
    SLOT(resizeColumn(QTreeWidgetItem*)) /*, Qt::QueuedConnection*/);
}

//----------------------------------------------------------------------------
void pqPluginDialog::populatePluginTree(
  QTreeWidget* pluginTree, vtkPVPluginsInformation* pluginList, bool remote)
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  pluginTree->blockSignals(true);
  pluginTree->clear();
  for (unsigned int cc = 0; cc < pluginList->GetNumberOfPlugins(); ++cc)
  {
    if (pm->isHidden(pluginList->GetPluginFileName(cc), remote))
    {
      continue;
    }
    QTreeWidgetItem* mNode = new QTreeWidgetItem(pluginTree, QTreeWidgetItem::UserType);
    QVariant vdata;
    vdata.setValue(cc);
    mNode->setData(NameCol, Qt::UserRole, vdata);

    Qt::ItemFlags parentFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    mNode->setText(NameCol, pluginList->GetPluginName(cc));
    mNode->setFlags(parentFlags);
    mNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    this->addInfoNodes(mNode, pluginList, cc, remote);
  }
  pluginTree->blockSignals(false);
}

//----------------------------------------------------------------------------
vtkPVPluginsInformation* pqPluginDialog::getPluginInfo(
  QTreeWidgetItem* pluginNode, unsigned int& index)
{
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  vtkPVPluginsInformation* info = pm->loadedExtensions(
    this->Server, (pluginNode->treeWidget() == this->Ui->remotePlugins) ? true : false);

  index = pluginNode ? pluginNode->data(NameCol, Qt::UserRole).toUInt() : 0;

  if (info && info->GetNumberOfPlugins() > index)
  {
    return info;
  }
  index = 0;
  return NULL;
}

//----------------------------------------------------------------------------
void pqPluginDialog::addInfoNodes(QTreeWidgetItem* pluginNode, vtkPVPluginsInformation* plInfo,
  unsigned int index, bool vtkNotUsed(remote))
{
  Qt::ItemFlags infoFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  // set icon hint
  if (plInfo->GetPluginLoaded(index))
  {
    pluginNode->setText(ValueCol, "Loaded");
    if (plInfo->GetPluginStatusMessage(index))
    {
      pluginNode->setIcon(ValueCol, QIcon(":/pqWidgets/Icons/warning.png"));
    }
  }
  else
  {
    pluginNode->setText(ValueCol, "Not Loaded");
  }

  QVariant vdata;
  vdata.setValue(index);

  QStringList infoText;
  // Version
  infoText << tr("Version") << tr(plInfo->GetPluginVersion(index));
  QTreeWidgetItem* infoNode = new QTreeWidgetItem(pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  infoNode->setData(NameCol, Qt::UserRole, vdata);

  // Description
  if (strlen(plInfo->GetDescription(index)) > 0)
  {
    infoText.clear();
    infoText << tr("Description");
    infoText << tr(plInfo->GetDescription(index));
    infoNode = new QTreeWidgetItem(pluginNode, infoText);
    infoNode->setFlags(infoFlags);
    infoNode->setToolTip(ValueCol, tr(plInfo->GetDescription(index)));
    infoNode->setData(NameCol, Qt::UserRole, vdata);
  }

  // Location
  infoText.clear();
  infoText << tr("Location") << tr(plInfo->GetPluginFileName(index));
  infoNode = new QTreeWidgetItem(pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  infoNode->setToolTip(ValueCol, tr(plInfo->GetPluginFileName(index)));
  infoNode->setData(NameCol, Qt::UserRole, vdata);

  // Depended Plugins
  if (plInfo->GetRequiredPlugins(index))
  {
    infoText.clear();
    infoText << tr("Required Plugins");
    infoText << tr(plInfo->GetRequiredPlugins(index));
    infoNode = new QTreeWidgetItem(pluginNode, infoText);
    infoNode->setFlags(infoFlags);
    infoNode->setToolTip(ValueCol, tr(plInfo->GetRequiredPlugins(index)));
    infoNode->setData(NameCol, Qt::UserRole, vdata);
  }

  // Load status
  infoText.clear();
  infoText << tr("Status");
  infoText << this->getStatusText(plInfo, index);
  infoNode = new QTreeWidgetItem(pluginNode, infoText);
  infoNode->setFlags(infoFlags);
  if (plInfo->GetPluginStatusMessage(index) != NULL)
  {
    infoNode->setToolTip(ValueCol, tr(plInfo->GetPluginStatusMessage(index)));
  }
  infoNode->setData(NameCol, Qt::UserRole, vdata);

  // AutoLoad setting
  infoText.clear();
  infoText << tr("Auto Load") << tr("");
  infoNode = new QTreeWidgetItem(pluginNode, infoText);
  infoNode->setFlags(infoFlags | Qt::ItemIsUserCheckable);
  infoNode->setCheckState(ValueCol, plInfo->GetAutoLoad(index) ? Qt::Checked : Qt::Unchecked);
  infoNode->setData(NameCol, Qt::UserRole, vdata);
}

//----------------------------------------------------------------------------
void pqPluginDialog::onPluginItemChanged(QTreeWidgetItem* item, int col)
{
  // Called when user toggles the checkbox for auto-load.
  if (item && col == ValueCol)
  {
    unsigned int index = 0;
    vtkPVPluginsInformation* plInfo =
      this->getPluginInfo(static_cast<QTreeWidgetItem*>(item->parent()), index);
    if (plInfo)
    {
      bool autoLoad = item->checkState(col) == Qt::Checked;
      // the vtkSMPluginManager ensures that the auto-load flag is preserved
      // even when the plugininfo is updated as new plugins are loaded.
      plInfo->SetAutoLoadAndForce(index, autoLoad);
    }
  }
}

//----------------------------------------------------------------------------
void pqPluginDialog::loadSelectedPlugins(
  QList<QTreeWidgetItem*> selItems, pqServer* server, bool remote)
{
  this->LoadingMultiplePlugins = true;
  for (int i = 0; i < selItems.count(); i++)
    foreach (QTreeWidgetItem* item, selItems)
    {
      unsigned int index = 0;
      vtkPVPluginsInformation* plInfo = this->getPluginInfo(item, index);
      if (plInfo && plInfo->GetPluginFileName(index) && !plInfo->GetPluginLoaded(index))
      {
        this->loadPlugin(server, QString(plInfo->GetPluginFileName(index)), remote);
      }
    }
  this->LoadingMultiplePlugins = false;
  this->refresh();
}

//----------------------------------------------------------------------------
void pqPluginDialog::onLoadSelectedRemotePlugin()
{
  this->loadSelectedPlugins(this->Ui->remotePlugins->selectedItems(), this->Server, true);
}

//----------------------------------------------------------------------------
void pqPluginDialog::onLoadSelectedLocalPlugin()
{
  this->loadSelectedPlugins(this->Ui->localPlugins->selectedItems(), this->Server, false);
}

//----------------------------------------------------------------------------
void pqPluginDialog::removeSelectedPlugins(
  QList<QTreeWidgetItem*> selItems, pqServer* server, bool remote)
{
  for (int i = 0; i < selItems.count(); i++)
  {
    unsigned int index = 0;
    vtkPVPluginsInformation* plInfo = this->getPluginInfo(selItems.value(i), index);
    if (plInfo && plInfo->GetPluginFileName(index))
    {
      this->removePlugin(server, QString(plInfo->GetPluginFileName(index)), remote);
    }
  }
  this->refresh();
}

//----------------------------------------------------------------------------
void pqPluginDialog::onRemoveSelectedRemotePlugin()
{
  this->removeSelectedPlugins(this->Ui->remotePlugins->selectedItems(), this->Server, true);
  this->onRemoteSelectionChanged();
}

//----------------------------------------------------------------------------
void pqPluginDialog::onRemoveSelectedLocalPlugin()
{
  this->removeSelectedPlugins(this->Ui->localPlugins->selectedItems(), this->Server, false);
  this->onLocalSelectionChanged();
}

//----------------------------------------------------------------------------
void pqPluginDialog::onRemoteSelectionChanged()
{
  this->updateEnableState(
    this->Ui->remotePlugins, this->Ui->removeRemote, this->Ui->loadSelected_Remote);
}
//----------------------------------------------------------------------------
void pqPluginDialog::onLocalSelectionChanged()
{
  this->updateEnableState(
    this->Ui->localPlugins, this->Ui->removeLocal, this->Ui->loadSelected_Local);
}
//----------------------------------------------------------------------------
void pqPluginDialog::updateEnableState(
  QTreeWidget* pluginTree, QPushButton* removeButton, QPushButton* loadButton)
{
  bool shouldEnableLoad = false;
  int num = pluginTree->selectedItems().count();
  for (int i = 0; i < num; i++)
  {
    QTreeWidgetItem* pluginNode = pluginTree->selectedItems().value(i);
    unsigned int index = 0;
    vtkPVPluginsInformation* plInfo = this->getPluginInfo(pluginNode, index);
    if (plInfo && !plInfo->GetPluginLoaded(index))
    {
      shouldEnableLoad = true;
      break;
    }
  }

  loadButton->setEnabled(shouldEnableLoad);
  removeButton->setEnabled(num > 0 ? 1 : 0);
}

//----------------------------------------------------------------------------
void pqPluginDialog::resizeColumn(QTreeWidgetItem* item)
{
  item->treeWidget()->resizeColumnToContents(ValueCol);
}

//----------------------------------------------------------------------------
QString pqPluginDialog::getStatusText(vtkPVPluginsInformation* plInfo, unsigned int cc)
{
  QString text;
  if (plInfo->GetPluginStatusMessage(cc))
  {
    text = plInfo->GetPluginStatusMessage(cc);
  }
  else
  {
    text = plInfo->GetPluginLoaded(cc) ? "Loaded" : "Not Loaded";
  }
  return text;
}
