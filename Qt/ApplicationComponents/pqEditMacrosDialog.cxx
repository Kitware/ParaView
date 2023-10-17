// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditMacrosDialog.h"
#include "ui_pqEditMacrosDialog.h"

// pqCore
#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPVApplicationCore.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonManager.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonTabWidget.h"

// Qt
#include <QMessageBox>
#include <QScopedPointer>
#include <QShortcut>
#include <QStringList>
#include <QTreeWidgetItem>

//----------------------------------------------------------------------------
struct pqEditMacrosDialog::pqInternals
{
  pqInternals()
    : Ui(new Ui::pqEditMacrosDialog)
  {
  }

  QScopedPointer<Ui::pqEditMacrosDialog> Ui;
};

//----------------------------------------------------------------------------
pqEditMacrosDialog::pqEditMacrosDialog(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqInternals)
{
  this->Internals->Ui->setupUi(this);
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  this->Internals->Ui->macrosTree->setAcceptDrops(false);
  this->Internals->Ui->macrosTree->sortByColumn(0, Qt::AscendingOrder);
  this->Internals->Ui->macrosTree->setHeaderLabels(QStringList() << tr("Macro Name"));

  QObject::connect(pqPVApplicationCore::instance()->pythonManager()->macroSupervisor(),
    &pqPythonMacroSupervisor::onAddedMacro, this, &pqEditMacrosDialog::populateTree);

  QObject::connect(
    this->Internals->Ui->add, &QToolButton::released, this, &pqEditMacrosDialog::onAddPressed);

  QObject::connect(
    this->Internals->Ui->edit, &QToolButton::released, this, &pqEditMacrosDialog::onEditPressed);
  this->connect(this->Internals->Ui->macrosTree, &QTreeWidget::itemDoubleClicked, this,
    &pqEditMacrosDialog::onEditPressed);

  QObject::connect(this->Internals->Ui->remove, &QToolButton::released, this,
    &pqEditMacrosDialog::onRemovePressed);
  QShortcut* deleteShortcut =
    new QShortcut(QKeySequence(Qt::Key_Delete), this->Internals->Ui->macrosTree);
  QObject::connect(
    deleteShortcut, &QShortcut::activated, this, &pqEditMacrosDialog::onRemovePressed);

  QObject::connect(this->Internals->Ui->removeAll, &QToolButton::released, this,
    &pqEditMacrosDialog::onRemoveAllPressed);

  QObject::connect(this->Internals->Ui->searchBox, &pqSearchBox::textChanged, this,
    &pqEditMacrosDialog::onSearchTextChanged);

  this->connect(this->Internals->Ui->macrosTree, &QTreeWidget::itemSelectionChanged, this,
    &pqEditMacrosDialog::updateUIState);

  this->populateTree();
}

//----------------------------------------------------------------------------
pqEditMacrosDialog::~pqEditMacrosDialog() = default;

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onAddPressed()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(),
    tr("Open Python File to create a Macro:"), QString(),
    tr("Python Files") + QString(" (*.py);;") + tr("All Files") + QString(" (*)"), false, false);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->addMacro(fileDialog.getSelectedFiles()[0], fileDialog.getSelectedLocation());
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onEditPressed()
{
  QList<QTreeWidgetItem*> selected = this->Internals->Ui->macrosTree->selectedItems();
  for (auto item : selected)
  {
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->editMacro(item->data(0, Qt::UserRole).toString());
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onRemovePressed()
{
  QList<QTreeWidgetItem*> selected = this->Internals->Ui->macrosTree->selectedItems();
  if (selected.empty())
  {
    return;
  }

  this->deleteItems(selected);

  // Python Manager can't be nullptr as this dialog is built only when Python is enabled
  pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  pythonManager->updateMacroList();
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onRemoveAllPressed()
{
  QMessageBox::StandardButton ret = QMessageBox::question(pqCoreUtilities::mainWidget(),
    QCoreApplication::translate("pqMacrosMenu", "Delete All"),
    QCoreApplication::translate("pqMacrosMenu", "All macros will be deleted. Are you sure?"));
  if (ret == QMessageBox::StandardButton::Yes)
  {
    // The script editor shows macros about to be deleted. remove those tabs.
    pqPythonTabWidget* const tWidget =
      pqPythonScriptEditor::getUniqueInstance()->findChild<pqPythonTabWidget*>();
    for (int i = tWidget->count() - 1; i >= 0; --i)
    {
      Q_EMIT tWidget->tabCloseRequested(i);
    }

    // remove user Macros dir
    pqCoreUtilities::removeRecursively(pqPythonScriptEditor::getMacrosDir());
    this->Internals->Ui->macrosTree->clear();
    this->updateUIState();

    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->updateMacroList();
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onSearchTextChanged(const QString& pattern)
{
  auto root = this->Internals->Ui->macrosTree->invisibleRootItem();
  for (int i = 0; i < root->childCount(); ++i)
  {
    auto item = root->child(i);
    if (pattern.isEmpty())
    {
      item->setHidden(false);
    }
    else
    {
      item->setHidden(!item->text(0).contains(pattern, Qt::CaseInsensitive));
    }
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::createItem(QTreeWidgetItem* parent, const QString& fileName,
  const QString& displayName, QTreeWidgetItem* preceding)
{
  if (displayName.isEmpty())
  {
    return;
  }

  if (preceding == nullptr)
  {
    preceding = parent->child(parent->childCount() - 1);
  }

  QTreeWidgetItem* item = new QTreeWidgetItem(parent, preceding);
  item->setText(0, displayName);
  item->setData(0, Qt::UserRole, fileName);
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QString iconPath = pqPythonMacroSupervisor::iconPathFromFileName(fileName);
  if (!iconPath.isEmpty())
  {
    item->setIcon(0, QIcon(iconPath));
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::populateTree()
{
  this->Internals->Ui->macrosTree->clear();

  auto treeRoot = this->Internals->Ui->macrosTree->invisibleRootItem();
  auto macrosMap = pqPythonMacroSupervisor::getStoredMacros();

  for (auto macrosIter = macrosMap.cbegin(), end = macrosMap.cend(); macrosIter != end;
       ++macrosIter)
  {
    this->createItem(treeRoot, macrosIter.key(), macrosIter.value());
  }

  this->Internals->Ui->macrosTree->resizeColumnToContents(0);
  this->Internals->Ui->macrosTree->setCurrentItem(nullptr);
  this->updateUIState();
}

//----------------------------------------------------------------------------
bool pqEditMacrosDialog::treeHasItems()
{
  return this->Internals->Ui->macrosTree->topLevelItemCount() > 0;
}

//----------------------------------------------------------------------------
bool pqEditMacrosDialog::treeHasSelectedItems()
{
  return !this->Internals->Ui->macrosTree->selectedItems().isEmpty();
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqEditMacrosDialog::getSelectedItem()
{
  if (this->treeHasSelectedItems())
  {
    return this->Internals->Ui->macrosTree->selectedItems().first();
  }

  return this->Internals->Ui->macrosTree->invisibleRootItem();
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqEditMacrosDialog::getNearestItem(QTreeWidgetItem* item)
{
  auto above = this->Internals->Ui->macrosTree->itemAbove(item);
  auto below = this->Internals->Ui->macrosTree->itemBelow(item);

  bool itemIsLastChild = false;
  auto parent = item->parent();
  if (parent)
  {
    auto numberOfItems = parent->childCount();
    auto index = parent->indexOfChild(item);
    itemIsLastChild = index == numberOfItems - 1;
  }

  return itemIsLastChild ? above : below;
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::deleteItem(QTreeWidgetItem* item)
{
  pqPythonMacroSupervisor::hideFile(item->data(0, Qt::UserRole).toString());
  delete item;
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::deleteItems(const QList<QTreeWidgetItem*>& items)
{
  for (auto item : items)
  {
    if (item)
    {
      auto nextSelectedItem = this->getNearestItem(item);
      this->Internals->Ui->macrosTree->scrollToItem(nextSelectedItem);
      this->Internals->Ui->macrosTree->setCurrentItem(nextSelectedItem);
      this->deleteItem(item);
    }
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::updateUIState()
{
  const bool hasItems = this->treeHasItems();
  const bool hasSelectedItems = this->treeHasSelectedItems();

  this->Internals->Ui->edit->setEnabled(hasSelectedItems);
  this->Internals->Ui->remove->setEnabled(hasSelectedItems);
  this->Internals->Ui->removeAll->setEnabled(hasItems);
}
