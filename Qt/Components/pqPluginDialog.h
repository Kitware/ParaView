/*=========================================================================

   Program: ParaView
   Module:    pqPluginDialog.h

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

#ifndef _pqPluginDialog_h
#define _pqPluginDialog_h

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
  pqPluginDialog(pqServer* server, QWidget* p = 0);
  /**
  * destroy this dialog
  */
  ~pqPluginDialog() override;

public Q_SLOTS:
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

protected:
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

private:
  QScopedPointer<Ui::pqPluginDialog> Ui;
  pqServer* Server;
  bool LoadingMultiplePlugins;
};

#endif
