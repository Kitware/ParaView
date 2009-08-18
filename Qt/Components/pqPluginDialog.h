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

#include <QDialog>
#include <QPointer>
#include "pqComponentsExport.h"
#include "ui_pqPluginDialog.h"

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class pqServer;
class vtkPVPluginInformation;

class PQCOMPONENTS_EXPORT pqPluginDialog :
  public QDialog, private Ui::pqPluginDialog
{
  Q_OBJECT
  typedef QDialog base;
public:
  /// create this dialog with a parent
  pqPluginDialog(pqServer* server, QWidget* p=0);
  /// destroy this dialog
  ~pqPluginDialog();

public slots:
  void loadLocalPlugin();
  void loadRemotePlugin();
  
protected slots:
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
  QString loadPlugin(pqServer* server, bool remote);
  QString loadPlugin(pqServer* server, const QString& file, bool remote);
  void removePlugin(pqServer* server, const QString& file, bool remote);
  
  void setupTreeWidget(QTreeWidget* pluginTree);
  void populatePluginTree(QTreeWidget* pluginTree,
    QList< vtkPVPluginInformation* >& pluginList, bool remote);
  void createPluginNode(QTreeWidget* pluginTree,
    vtkPVPluginInformation* pluginInfo, bool remote);
  void addPluginInfo(QTreeWidgetItem* pluginNode, bool remote);
  void addInfoNodes(
    QTreeWidgetItem* pluginNode, vtkPVPluginInformation* plInfo, bool remote);
  vtkPVPluginInformation* getPluginInfo(QTreeWidgetItem* pluginNode)    ;
  void updateEnableState(QTreeWidget*, QPushButton* removeButton, QPushButton* loadButton);
  void loadSelectedPlugins(QList<QTreeWidgetItem*> selItems,
    pqServer* server, bool remote);
  void removeSelectedPlugins(QList<QTreeWidgetItem*> selItems,
    pqServer* server, bool remote);
  QString getStatusText(vtkPVPluginInformation* plInfo);

private:      
  
  pqServer* Server;
  bool LoadingMultiplePlugins;
};

#endif

