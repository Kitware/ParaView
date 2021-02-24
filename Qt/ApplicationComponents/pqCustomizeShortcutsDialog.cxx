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
#include <QSortFilterProxyModel>

namespace
{

class TreeItem
{
public:
  explicit TreeItem(const QString& name, const QString& fullKey, bool isLeaf, QAction* action,
    TreeItem* parentItem = nullptr)
    : Name(name)
    , FullKey(fullKey)
    , Action(action)
    , IsLeaf(isLeaf)
    , ParentItem(parentItem)
  {
    if (action)
    {
      this->KeySequence = action->shortcut().toString();
    }
  }

  ~TreeItem() { qDeleteAll(this->ChildItems); }

  void appendChild(TreeItem* child) { this->ChildItems.append(child); }

  TreeItem* child(int row) { return this->ChildItems[row]; }

  int childCount() const { return this->ChildItems.size(); }

  const QString& name() const { return this->Name; }

  const QString& settingsKey() const { return this->FullKey; }

  QKeySequence keySequence() const { return this->KeySequence; }

  void setKeySequence(const QKeySequence& shortcut)
  {
    this->KeySequence = shortcut.toString();
    this->Action->setShortcut(shortcut);
    pqSettings settings;
    settings.setValue(settingsKey(), shortcut);
  }

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

  QKeySequence defaultKeySequence()
  {
    if (!this->Action)
    {
      return QKeySequence();
    }
    QVariant shortcut = this->Action->property("ParaViewDefaultKeySequence");
    if (shortcut.isValid())
    {
      return shortcut.value<QKeySequence>();
    }
    return QKeySequence();
  }

private:
  QList<TreeItem*> ChildItems;
  QString Name;
  QString FullKey;
  QKeySequence KeySequence;
  QPointer<QAction> Action;
  bool IsLeaf;
  TreeItem* ParentItem;
};

class pqCustomizeShortcutsModel : public QAbstractItemModel
{
  static void buildTree(TreeItem* root, const QList<QAction*>& actions, pqSettings& settings)
  {
    for (QAction* action : actions)
    {
      QString actionName = pqCustomizeShortcutsDialog::getActionName(action);
      if (actionName.isEmpty())
      {
        continue;
      }
      actionName = actionName.replace("&", "");
      QString localSettingName = actionName.replace(" ", "_");
      QString settingsKey = QString("%1/%2").arg(settings.group()).arg(localSettingName);
      if (action->menu())
      {
        if ((root->name() == "Filters" || root->name() == "Sources") &&
          actionName != "Alphabetical")
        {
          // All filters are under alphabetical, but the actions are also in the
          // category menus. Ignore all but the alphabetical category to avoid duplicates.
          continue;
        }
        if (actionName != "Recent_Files" &&
          (!(root->name() == "Macros" && (actionName == "Edit..." || actionName == "Delete..."))))
        {
          TreeItem* myItem = new TreeItem(actionName, settingsKey, false, action, root);
          root->appendChild(myItem);
          settings.beginGroup(localSettingName);
          buildTree(myItem, action->menu()->actions(), settings);
          settings.endGroup();
        }
      }
      else
      {
        QString shortcut;
        if (settings.contains(localSettingName))
        {
          shortcut = settings.value(localSettingName).toString();
        }
        TreeItem* myItem = new TreeItem(actionName, settingsKey, true, action, root);
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
    TreeItem* treeItem = new TreeItem("", "", false, nullptr, nullptr);
    settings.beginGroup("pqCustomShortcuts");
    buildTree(treeItem, menuBar->actions(), settings);
    settings.endGroup();
    this->RootItem = treeItem;
  }

  ~pqCustomizeShortcutsModel() override { delete this->RootItem; }

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
    TreeItem* item = static_cast<TreeItem*>(idx.internalPointer());
    if (role == Qt::DisplayRole)
    {

      switch (idx.column())
      {
        case 0:
        {
          QString result = item->name();
          result.replace("_", " ");
          return result;
        }
        case 1:
          return item->keySequence();
      }
    }
    else if (role == Qt::FontRole)
    {
      switch (idx.column())
      {
        case 1:
          QFont font;
          if (item->keySequence() != item->defaultKeySequence())
          {
            font.setBold(true);
          }
          return font;
      }
    }
    return QVariant();
  }

  void setKeySequence(const QModelIndex& idx, const QKeySequence& shortcut)
  {
    if (!idx.isValid() || idx.column() != 1)
    {
      return;
    }
    TreeItem* item = static_cast<TreeItem*>(idx.internalPointer());
    if (!item || !item->isLeaf())
    {
      return;
    }
    if (shortcut != item->keySequence())
    {
      item->setKeySequence(shortcut);
      QVector<int> roles;
      roles << Qt::DisplayRole;
      Q_EMIT dataChanged(idx, idx, roles);
    }
  }

  void resetAll()
  {
    callOnAllLeaves(
      [this](const QModelIndex& leafIndex, TreeItem* item) {
        QKeySequence shortcut = item->defaultKeySequence();
        QModelIndex indexToChange = this->index(leafIndex.row(), 1, this->parent(leafIndex));
        this->setKeySequence(indexToChange, shortcut);
      },
      QModelIndex());
  }

private:
  template <typename T>
  void callOnAllLeaves(T func, const QModelIndex& index)
  {
    const int numRows = this->rowCount(index);
    for (int i = 0; i < numRows; ++i)
    {
      QModelIndex childIndex = this->index(i, 0, index);
      TreeItem* item = static_cast<TreeItem*>(childIndex.internalPointer());
      if (!item)
      {
        return;
      }
      if (item->isLeaf())
      {
        func(childIndex, item);
      }
      else
      {
        this->callOnAllLeaves(func, childIndex);
      }
    }
  }
  TreeItem* RootItem;
};

class FilterLeavesProxyModel : public QSortFilterProxyModel
{
  typedef QSortFilterProxyModel Superclass;

public:
  FilterLeavesProxyModel(QObject* p = nullptr)
    : Superclass(p)
  {
  }

  ~FilterLeavesProxyModel() override = default;

  bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
  {
    auto source = this->sourceModel();
    auto indexInQuestion = source->index(source_row, 0, source_parent);
    if (source->hasChildren(indexInQuestion))
    {
      const int numRows = source->rowCount(indexInQuestion);
      for (int i = 0; i < numRows; ++i)
      {
        if (filterAcceptsRow(i, indexInQuestion))
        {
          return true;
        }
      }
      return false;
    }
    else
    {
      return Superclass::filterAcceptsRow(source_row, source_parent);
    }
  }
};
}

class pqCustomizeShortcutsDialog::pqInternals
{
public:
  Ui::pqCustomizeShortcutsDialog Ui;
  QPointer<pqCustomizeShortcutsModel> Model;
  QPointer<QSortFilterProxyModel> FilterModel;

  pqInternals(pqCustomizeShortcutsDialog* self)
    : Model(new pqCustomizeShortcutsModel(self))
    , FilterModel(new FilterLeavesProxyModel(self))
  {
    Ui.setupUi(self);
    this->FilterModel->setSourceModel(this->Model);
    Ui.treeView->setModel(this->FilterModel);
    Ui.treeView->expandAll();
    Ui.treeView->resizeColumnToContents(0);
    Ui.searchBox->setAdvancedSearchEnabled(false);
  }
};

pqCustomizeShortcutsDialog::pqCustomizeShortcutsDialog(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals(this))
{
  connect(this->Internals->Ui.keySequenceEdit, &QKeySequenceEdit::editingFinished, this,
    &pqCustomizeShortcutsDialog::onEditingFinished);
  connect(this->Internals->Ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
    this, &pqCustomizeShortcutsDialog::onSelectionChanged);
  connect(this->Internals->Ui.clearButton, &QAbstractButton::clicked, this,
    &pqCustomizeShortcutsDialog::onClearShortcut);
  connect(this->Internals->Ui.resetButton, &QAbstractButton::clicked, this,
    &pqCustomizeShortcutsDialog::onResetShortcut);
  connect(this->Internals->Ui.resetAllButton, &QAbstractButton::clicked, this,
    &pqCustomizeShortcutsDialog::onResetAll);
  connect(this->Internals->Ui.recordButton, &QAbstractButton::clicked, this,
    [this]() { this->Internals->Ui.keySequenceEdit->setFocus(); });
  connect(this->Internals->Ui.searchBox, &pqSearchBox::textChanged, this, [this]() {
    QRegExp regex(this->Internals->Ui.searchBox->text(), Qt::CaseInsensitive);

    this->Internals->FilterModel->setFilterRegExp(regex);
    this->Internals->Ui.treeView->expandAll();
  });
  this->setWindowTitle("Customize Shortcuts");
  this->onSelectionChanged();
}

pqCustomizeShortcutsDialog::~pqCustomizeShortcutsDialog() = default;

void pqCustomizeShortcutsDialog::onEditingFinished()
{
  auto selectionModel = this->Internals->Ui.treeView->selectionModel();
  auto selectedList = selectionModel->selectedRows(1);
  if (selectedList.size() == 0)
  {
    return;
  }
  auto selected = selectedList[0];
  if (!selected.isValid())
  {
    return;
  }
  selected = this->Internals->FilterModel->mapToSource(selected);
  this->Internals->Model->setKeySequence(
    selected, this->Internals->Ui.keySequenceEdit->keySequence());
  // Update Reset/Clear buttons' enabled state
  this->onSelectionChanged();
}

void pqCustomizeShortcutsDialog::onSelectionChanged()
{
  auto setEnabled = [this](bool enable) {
    this->Internals->Ui.recordButton->setEnabled(enable);
    this->Internals->Ui.clearButton->setEnabled(enable);
    this->Internals->Ui.resetButton->setEnabled(enable);
    this->Internals->Ui.keySequenceEdit->setEnabled(enable);
  };
  auto selectionModel = this->Internals->Ui.treeView->selectionModel();
  auto selectedList = selectionModel->selectedRows(1);
  if (selectedList.size() == 0)
  {
    setEnabled(false);
    return;
  }
  auto selected = selectedList[0];
  if (!selected.isValid())
  {
    setEnabled(true);
    return;
  }
  selected = this->Internals->FilterModel->mapToSource(selected);
  QString keySequence = this->Internals->Model->data(selected, Qt::DisplayRole).toString();
  this->Internals->Ui.keySequenceEdit->setKeySequence(keySequence);
  TreeItem* item = static_cast<TreeItem*>(selected.internalPointer());
  setEnabled(item->isLeaf());
  // If the item does not have a default keysequence, Reset and Clear do
  // the same thing.  Disable Reset to avoid user confusion.
  // Also, disable Reset if the KeySequence is already the default.
  if (item->isLeaf())
  {
    if (item->defaultKeySequence() == QKeySequence() ||
      item->defaultKeySequence() == item->keySequence())
    {
      this->Internals->Ui.resetButton->setEnabled(false);
    }
    if (item->keySequence() == QKeySequence())
    {
      this->Internals->Ui.clearButton->setEnabled(false);
    }
  }
}

void pqCustomizeShortcutsDialog::onClearShortcut()
{
  auto selectionModel = this->Internals->Ui.treeView->selectionModel();
  auto selectedList = selectionModel->selectedRows(1);
  if (selectedList.size() == 0)
  {
    return;
  }
  auto selected = selectedList[0];
  if (!selected.isValid())
  {
    return;
  }
  selected = this->Internals->FilterModel->mapToSource(selected);
  this->Internals->Model->setKeySequence(selected, QKeySequence());
  this->Internals->Ui.keySequenceEdit->setKeySequence(QKeySequence());
  // Update Reset/Clear buttons' enabled state
  this->onSelectionChanged();
}

void pqCustomizeShortcutsDialog::onResetShortcut()
{
  auto selectionModel = this->Internals->Ui.treeView->selectionModel();
  auto selectedList = selectionModel->selectedRows(1);
  if (selectedList.size() == 0)
  {
    return;
  }
  auto selected = selectedList[0];
  if (!selected.isValid())
  {
    return;
  }
  selected = this->Internals->FilterModel->mapToSource(selected);
  TreeItem* item = static_cast<TreeItem*>(selected.internalPointer());
  QKeySequence shortcut = item->defaultKeySequence();
  this->Internals->Model->setKeySequence(selected, shortcut);
  this->Internals->Ui.keySequenceEdit->setKeySequence(shortcut);
  // Update Reset/Clear buttons' enabled state
  this->onSelectionChanged();
}

void pqCustomizeShortcutsDialog::onResetAll()
{
  this->Internals->Model->resetAll();
  this->onSelectionChanged();
}

QString pqCustomizeShortcutsDialog::getActionName(QAction* action)
{
  QString objectName = action->objectName();
  if (objectName == "actionEditUndo")
  {
    return "Undo";
  }
  else if (objectName == "actionEditRedo")
  {
    return "Redo";
  }
  else if (objectName == "actionToolsStartStopTrace")
  {
    return "StartStopTrace";
  }
  else
  {
    return action->text();
  }
}
