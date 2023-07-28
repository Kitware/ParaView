// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTreeViewSelectionHelper.h"

#include "pqHeaderView.h"

#include <QItemSelection>
#include <QLineEdit>
#include <QMenu>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QtDebug>

#include <QTableView>
#include <QTreeView>

namespace
{
int genericColumnAt(QAbstractItemView* item, int x)
{
  auto treeView = dynamic_cast<QTreeView*>(item);
  auto tableView = dynamic_cast<QTableView*>(item);

  return treeView ? treeView->columnAt(x) : tableView ? tableView->columnAt(x) : -1;
}

QHeaderView* genericHeader(QAbstractItemView* item)
{
  auto treeView = dynamic_cast<QTreeView*>(item);
  auto tableView = dynamic_cast<QTableView*>(item);

  return treeView ? treeView->header() : tableView ? tableView->horizontalHeader() : nullptr;
}

void updateFilter(QAbstractItemView* tree, int section, const QString& txt)
{
  auto model = tree->model();
  auto header = genericHeader(tree);
  auto pqheader = qobject_cast<pqHeaderView*>(header);
  auto sfmodel = qobject_cast<QSortFilterProxyModel*>(model);

  if (sfmodel)
  {
    sfmodel->setFilterRegularExpression(
      QRegularExpression(txt, QRegularExpression::CaseInsensitiveOption));
    sfmodel->setFilterKeyColumn(section);
  }
  if (pqheader && sfmodel)
  {
    if (txt.isEmpty())
    {
      pqheader->removeCustomIndicatorIcon("remove-filter");
    }
    else
    {
      pqheader->addCustomIndicatorIcon(QIcon(":/QtWidgets/Icons/pqDelete.svg"), "remove-filter");
    }
  }
}
}

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::pqTreeViewSelectionHelper(QAbstractItemView* tree, bool customIndicator)
  : Superclass(tree)
  , TreeView(tree)
  , Filterable(true)
{
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);

  auto pqheader = qobject_cast<pqHeaderView*>(genericHeader(tree));
  if (customIndicator && pqheader)
  {
    pqheader->setCustomIndicatorShown(true);
    pqheader->addCustomIndicatorIcon(QIcon(":/QtWidgets/Icons/pqShowMenu.svg"), "menu");
    QObject::connect(pqheader, &pqHeaderView::customIndicatorClicked,
      [this, tree](int section, const QPoint& pt, const QString& role) {
        if (role == "menu")
        {
          this->showContextMenu(section, pt);
        }
        else if (role == "remove-filter")
        {
          updateFilter(tree, section, QString());
        }
      });
  }
  QObject::connect(tree, &QAbstractItemView::customContextMenuRequested,
    [this, tree](const QPoint& pt) { this->showContextMenu(genericColumnAt(tree, pt.x()), pt); });
}

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::~pqTreeViewSelectionHelper() = default;

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::setSelectedItemsCheckState(Qt::CheckState state)
{
  // Change all checkable items in the this->Selection to match the new
  // check state.
  auto model = this->TreeView->model();
  for (const QModelIndex& idx : this->TreeView->selectionModel()->selectedIndexes())
  {
    const Qt::ItemFlags flags = model->flags(idx);
    if ((flags & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
    {
      model->setData(idx, state, Qt::CheckStateRole);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::buildupMenu(QMenu& menu, int section, const QPoint& pos)
{
  Q_UNUSED(pos);

  auto tree = this->TreeView;
  auto model = tree->model();
  auto header = genericHeader(tree);
  auto sfmodel = qobject_cast<QSortFilterProxyModel*>(model);

  QList<QModelIndex> selectedIndexes;
  for (const QModelIndex& idx : this->TreeView->selectionModel()->selectedIndexes())
  {
    if (idx.column() == section)
    {
      selectedIndexes.push_back(idx);
    }
  }

  const bool isUserCheckable =
    model->headerData(section, header->orientation(), Qt::CheckStateRole).isValid();
  QModelIndex itemIndex = this->TreeView->indexAt(pos);

  bool isItemFilterable = false;
  bool isItemSortable = false;
  if (sfmodel != nullptr)
  {
    isItemFilterable = model->data(itemIndex, sfmodel->filterRole()).isValid();
    isItemSortable = model->data(itemIndex, sfmodel->sortRole()).isValid();
  }

  const int selectionCount = selectedIndexes.size();
  const int rowCount = model->rowCount();

  QLineEdit* searchLineEdit = nullptr;
  if (this->Filterable && isItemFilterable)
  {
    if (auto filterActn = new QWidgetAction(&menu))
    {
      searchLineEdit = new QLineEdit(&menu);
      searchLineEdit->setPlaceholderText(tr("Filter items (regex)"));
      searchLineEdit->setClearButtonEnabled(true);
      searchLineEdit->setText(sfmodel->filterRegularExpression().pattern());

      auto container = new QWidget(&menu);
      auto l = new QVBoxLayout(container);
      // ideally, I'd like to place the QLineEdit inline with rest of the menu
      // actions, but not sure how to do it.
      // const auto xpos = 28; //menu.style()->pixelMetric(QStyle::PM_MenuHMargin);
      l->setContentsMargins(2, 2, 2, 2);
      l->addWidget(searchLineEdit);

      // Close the QMenu on return press for an easier usage
      QObject::connect(searchLineEdit, &QLineEdit::returnPressed, &menu, &QMenu::close);

      QObject::connect(searchLineEdit, &QLineEdit::textChanged,
        [tree, section](const QString& txt) { updateFilter(tree, section, txt); });

      filterActn->setDefaultWidget(container);
      menu.addAction(filterActn);
    }
    menu.addSeparator();
  }

  if (isUserCheckable)
  {
    if (auto actn =
          menu.addAction(QIcon(":/pqWidgets/Icons/pqChecked.svg"), tr("Check highlighted items")))
    {
      actn->setEnabled(selectionCount > 0);
      QObject::connect(
        actn, &QAction::triggered, [this](bool) { this->setSelectedItemsCheckState(Qt::Checked); });
    }

    if (auto actn = menu.addAction(
          QIcon(":/pqWidgets/Icons/pqUnchecked.svg"), tr("Uncheck highlighted items")))
    {
      actn->setEnabled(selectionCount > 0);
      QObject::connect(actn, &QAction::triggered,
        [this](bool) { this->setSelectedItemsCheckState(Qt::Unchecked); });
    }
  }

  if (isItemSortable)
  {
    if (isUserCheckable)
    {
      menu.addSeparator();
    }

    auto order = Qt::AscendingOrder;
    if (sfmodel->sortColumn() != -1 && sfmodel->sortOrder() != Qt::DescendingOrder)
    {
      order = Qt::DescendingOrder;
    }

    if (auto actn = order == Qt::AscendingOrder
        ? menu.addAction(QIcon(":/pqWidgets/Icons/pqSortAscend.svg"), tr("Sort (ascending)"))
        : menu.addAction(QIcon(":/pqWidgets/Icons/pqSortDescend.svg"), tr("Sort (descending)")))
    {
      actn->setEnabled(rowCount > 0);
      QObject::connect(actn, &QAction::triggered, [order, section, sfmodel, header](bool) {
        sfmodel->sort(section, order);
        header->setSortIndicatorShown(true);
        header->setSortIndicator(section, order);
      });
    }

    if (auto actn = menu.addAction(QIcon(":/pqWidgets/Icons/pqClearSort.svg"), tr("Clear sorting")))
    {
      actn->setEnabled(sfmodel->sortColumn() != -1);
      QObject::connect(actn, &QAction::triggered, [order, sfmodel, header](bool) {
        // this clears the indicator, and sets the underlying model to not sort
        sfmodel->sort(-1, order);
        header->setSortIndicator(-1, order);
      });
    }
  }
  if (searchLineEdit)
  {
    searchLineEdit->setFocus();
  }
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::showContextMenu(int section, const QPoint& pos)
{
  QMenu menu;
  menu.setObjectName("TreeViewCheckMenu");

  this->buildupMenu(menu, section, pos);
  if (menu.isEmpty())
  {
    return;
  }

  menu.exec(this->TreeView->mapToGlobal(pos));
}
