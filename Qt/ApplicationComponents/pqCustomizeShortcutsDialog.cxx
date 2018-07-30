/*=========================================================================

   Program: ParaView
   Module:    pqCustomizeShortcutsDialog.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqCustomizeShortcutsDialog.h"

#include "ui_pqCustomizeShortcutsDialog.h"

#include "pqCoreUtilities.h"
#include "pqSettings.h"

#include <QAbstractItemModel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

namespace
{

class TreeItem
{
public:
  explicit TreeItem(const QString& name, const QString& fullKey, bool isLeaf,
    const QKeySequence& keySequence, TreeItem* parentItem = nullptr)
    : Name(name)
    , FullKey(fullKey)
    , KeySequence(keySequence)
    , IsLeaf(isLeaf)
    , ParentItem(parentItem)
  {
  }

  ~TreeItem() { qDeleteAll(this->ChildItems); }

  void appendChild(TreeItem* child) { this->ChildItems.append(child); }

  TreeItem* child(int row) { return this->ChildItems[row]; }

  int childCount() const { return this->ChildItems.size(); }

  const QString& name() const { return this->Name; }

  const QString& settingsKey() const { return this->FullKey; }

  const QKeySequence keySequence() const { return this->KeySequence; }

  bool isLeaf() const { return this->IsLeaf; }

  int row() const
  {
    if (this->ParentItem)
    {
      return this->ParentItem->ChildItems.indexOf(const_cast<TreeItem*>(this));
    }
    return 0;
  }

  TreeItem* parentItem() { return this->ParentItem; }

private:
  QList<TreeItem*> ChildItems;
  QString Name;
  QString FullKey;
  QKeySequence KeySequence;
  bool IsLeaf;
  TreeItem* ParentItem;
};

class pqCustomizeShortcutsModel : public QAbstractItemModel
{
  static void buildTree(TreeItem* root, const QList<QAction*>& actions, pqSettings& settings)
  {
    for (QAction* action : actions)
    {
      QString actionName = action->text();
      if (actionName.isEmpty())
      {
        continue;
      }
      actionName = actionName.replace("&", "");
      QString localSettingName = actionName.replace(" ", "");
      QString settingsKey = QString("%1/%2").arg(settings.group()).arg(localSettingName);
      if (action->menu())
      {
        TreeItem* myItem = new TreeItem(actionName, settingsKey, false, QKeySequence(), root);
        root->appendChild(myItem);
        settings.beginGroup(localSettingName);
        buildTree(myItem, action->menu()->actions(), settings);
        settings.endGroup();
      }
      else
      {
        QString shortcut;
        if (settings.contains(localSettingName))
        {
          shortcut = settings.value(localSettingName).toString();
        }
        shortcut = action->shortcut().toString();
        TreeItem* myItem = new TreeItem(actionName, settingsKey, true, shortcut, root);
        root->appendChild(myItem);
      }
    }
  }

public:
  pqCustomizeShortcutsModel(QObject* p)
    : QAbstractItemModel(p)
  {
    pqSettings settings;
    auto mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
    auto menuBar = mainWindow->menuBar();
    TreeItem* treeItem = new TreeItem("", "", false, QKeySequence(), nullptr);
    settings.beginGroup("pqCustomShortcuts");
    buildTree(treeItem, menuBar->actions(), settings);
    settings.endGroup();
    this->RootItem = treeItem;
  }

  ~pqCustomizeShortcutsModel() { delete this->RootItem; }

  QModelIndex index(int row, int column, const QModelIndex& parentIndex) const override
  {
    if (!hasIndex(row, column, parentIndex))
      return QModelIndex();

    TreeItem* parentItem;

    if (!parentIndex.isValid())
    {
      parentItem = this->RootItem;
    }
    else
    {
      parentItem = static_cast<TreeItem*>(parentIndex.internalPointer());
    }

    TreeItem* childItem = parentItem->child(row);
    if (childItem)
    {
      return createIndex(row, column, childItem);
    }
    else
    {
      return QModelIndex();
    }
  }

  QModelIndex parent(const QModelIndex& idx) const override
  {
    if (!idx.isValid())
    {
      return QModelIndex();
    }

    TreeItem* childItem = static_cast<TreeItem*>(idx.internalPointer());
    TreeItem* parentItem = childItem->parentItem();

    if (parentItem == this->RootItem)
    {
      return QModelIndex();
    }
    return createIndex(parentItem->row(), 0, parentItem);
  }

  int rowCount(const QModelIndex& parentIndex) const override
  {
    TreeItem* parentItem;
    if (parentIndex.column() > 0)
    {
      return 0;
    }

    if (!parentIndex.isValid())
    {
      parentItem = this->RootItem;
    }
    else
    {
      parentItem = static_cast<TreeItem*>(parentIndex.internalPointer());
    }
    return parentItem->childCount();
  }

  int columnCount(const QModelIndex&) const override { return 2; }

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      switch (section)
      {
        case 0:
          return "Action";
        case 1:
          return "Shortcut";
      }
    }
    return QVariant();
  }

  QVariant data(const QModelIndex& idx, int role) const override
  {
    if (!idx.isValid())
    {
      return QVariant();
    }
    if (role != Qt::DisplayRole)
    {
      return QVariant();
    }
    TreeItem* item = static_cast<TreeItem*>(idx.internalPointer());

    switch (idx.column())
    {
      case 0:
        return item->name();
      case 1:
        return item->keySequence();
    }
    return QVariant();
  }

private:
  TreeItem* RootItem;
};
}

class pqCustomizeShortcutsDialog::pqInternals
{
  Ui::pqCustomizeShortcutsDialog Ui;
  QPointer<pqCustomizeShortcutsModel> Model;

public:
  pqInternals(pqCustomizeShortcutsDialog* self)
    : Model(new pqCustomizeShortcutsModel(self))
  {
    Ui.setupUi(self);
    Ui.treeView->setModel(this->Model);
    Ui.treeView->expandAll();
    Ui.treeView->resizeColumnToContents(0);
  }
};

pqCustomizeShortcutsDialog::pqCustomizeShortcutsDialog(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals(this))
{
}

pqCustomizeShortcutsDialog::~pqCustomizeShortcutsDialog()
{
}

void pqCustomizeShortcutsDialog::onEditingFinished()
{
}

void pqCustomizeShortcutsDialog::onApply()
{
}
