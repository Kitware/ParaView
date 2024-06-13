// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginDialog_h
#define pqPluginDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QPointer>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class pqServer;
class vtkPVPluginInformation;
class vtkPVPluginsInformation;

namespace Ui
{
class pqPluginDialog;
}

class PQCOMPONENTS_EXPORT pqPluginDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  /**
   * create this dialog with a parent
   */
  pqPluginDialog(pqServer* server, QWidget* p = nullptr);
  /**
   * destroy this dialog
   */
  ~pqPluginDialog() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void loadLocalPlugin();
  void loadRemotePlugin();

protected Q_SLOTS:
  void onPluginItemChanged(QTreeWidgetItem*, int);
  void onRefresh();
  void refresh();
  void onLoadSelectedRemotePlugin();
  void onLoadSelectedLocalPlugin();
  void onRemoveSelectedRemotePlugin();
  void onRemoveSelectedLocalPlugin();
  void onRemoteSelectionChanged();
  void onLocalSelectionChanged();
  void resizeColumn(QTreeWidgetItem*);

protected: // NOLINT(readability-redundant-access-specifiers)
  void refreshLocal();
  void refreshRemote();
  void loadPlugin(pqServer* server, bool remote);
  void loadPlugin(pqServer* server, const QString& file, bool remote);
  void removePlugin(pqServer* server, const QString& file, bool remote);

  void setupTreeWidget(QTreeWidget* pluginTree);
  void populatePluginTree(
    QTreeWidget* pluginTree, vtkPVPluginsInformation* pluginList, bool remote);
  void addInfoNodes(
    QTreeWidgetItem* pluginNode, vtkPVPluginsInformation* plInfo, unsigned int index, bool remote);
  vtkPVPluginsInformation* getPluginInfo(QTreeWidgetItem* pluginNode, unsigned int& index);
  void updateEnableState(QTreeWidget*, QPushButton* removeButton, QPushButton* loadButton);
  void loadSelectedPlugins(QList<QTreeWidgetItem*> selItems, pqServer* server, bool remote);
  void removeSelectedPlugins(QList<QTreeWidgetItem*> selItems, pqServer* server, bool remote);
  QString getStatusText(vtkPVPluginsInformation* plInfo, unsigned int cc);

private Q_SLOTS:
  ///@{
  /**
   * Internal slots called when the a add plugin config file button is pressed
   */
  void onAddPluginConfigRemote();
  void onAddPluginConfigLocal();
  ///@}

private: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Internal methods called to add a plugin config file to the plugin manager
   */
  void addPluginConfigFile(bool remote);
  void addPluginConfigFile(const QString& config, bool remote);
  ///@}

  QScopedPointer<Ui::pqPluginDialog> Ui;
  pqServer* Server;
  bool LoadingMultiplePlugins;
};

#endif
