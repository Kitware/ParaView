/*=========================================================================

   Program: ParaView
   Module:    pqTreeViewSelectionHelper.cxx

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
#include "pqTreeViewSelectionHelper.h"

#include "pqHeaderView.h"

#include <QItemSelection>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QtDebug>

namespace
{
void updateFilter(QTreeView* tree, int section, const QString& txt)
{
  auto model = tree->model();
  auto header = tree->header();
  auto pqheader = qobject_cast<pqHeaderView*>(header);
  auto sfmodel = qobject_cast<QSortFilterProxyModel*>(model);

  if (sfmodel)
  {
    sfmodel->setFilterRegExp(QRegExp(txt, Qt::CaseInsensitive, QRegExp::Wildcard));
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
      pqheader->addCustomIndicatorIcon(QIcon(":/QtWidgets/Icons/pqDelete24.png"), "remove-filter");
    }
  }
}
}

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::pqTreeViewSelectionHelper(QTreeView* tree)
  : Superclass(tree)
  , TreeView(tree)
  , Filterable(true)
{
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);

  if (auto pqheader = qobject_cast<pqHeaderView*>(tree->header()))
  {
    pqheader->setCustomIndicatorShown(true);
    pqheader->addCustomIndicatorIcon(
      QIcon(":/QtWidgets/Icons/outline_arrow_drop_down_circle_black_24dp.png"), "menu");
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
  QObject::connect(tree, &QTreeView::customContextMenuRequested,
    [this, tree](const QPoint& pt) { this->showContextMenu(tree->columnAt(pt.x()), pt); });
}

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::~pqTreeViewSelectionHelper()
{
}

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
void pqTreeViewSelectionHelper::showContextMenu(int section, const QPoint& pos)
{
  auto tree = this->TreeView;
  auto model = tree->model();
  auto header = tree->header();
  auto sfmodel = qobject_cast<QSortFilterProxyModel*>(model);

  QList<QModelIndex> selectedIndexes;
  for (const QModelIndex& idx : this->TreeView->selectionModel()->selectedIndexes())
  {
    if (idx.column() == section)
    {
      selectedIndexes.push_back(idx);
    }
  }

  const bool user_checkable =
    model->headerData(section, header->orientation(), Qt::CheckStateRole).isValid();
  const int selectionCount = selectedIndexes.size();
  const int rowCount = model->rowCount();

  QMenu menu;
  menu.setObjectName("TreeViewCheckMenu");
  if (this->Filterable && sfmodel != nullptr)
  {
    if (auto filterActn = new QWidgetAction(&menu))
    {
      auto ledit = new QLineEdit(&menu);
      ledit->setPlaceholderText("Filter items");
      ledit->setClearButtonEnabled(true);
      ledit->setText(sfmodel->filterRegExp().pattern());

      auto container = new QWidget(&menu);
      auto l = new QVBoxLayout(container);
      // ideally, I'd like to place the QLineEdit inline with rest of the menu
      // actions, but not sure how to do it.
      // const auto xpos = 28; //menu.style()->pixelMetric(QStyle::PM_MenuHMargin);
      l->setContentsMargins(2, 2, 2, 2);
      l->addWidget(ledit);
      QObject::connect(ledit, &QLineEdit::textChanged,
        [tree, section](const QString& txt) { updateFilter(tree, section, txt); });

      filterActn->setDefaultWidget(container);
      menu.addAction(filterActn);
    }
    menu.addSeparator();
  }

  if (user_checkable)
  {
    if (auto actn = menu.addAction("Check highlighted items"))
    {
      actn->setEnabled(selectionCount > 0);
      QObject::connect(
        actn, &QAction::triggered, [this](bool) { this->setSelectedItemsCheckState(Qt::Checked); });
    }

    if (auto actn = menu.addAction("Uncheck highlighted items"))
    {
      actn->setEnabled(selectionCount > 0);
      QObject::connect(actn, &QAction::triggered,
        [this](bool) { this->setSelectedItemsCheckState(Qt::Unchecked); });
    }
  }

  if (sfmodel != nullptr)
  {
    if (user_checkable)
    {
      menu.addSeparator();
    }

    auto order = Qt::AscendingOrder;
    if (sfmodel->sortColumn() != -1 && sfmodel->sortOrder() != Qt::DescendingOrder)
    {
      order = Qt::DescendingOrder;
    }

    if (auto actn =
          menu.addAction(order == Qt::AscendingOrder ? "Sort (ascending)" : "Sort (descending)"))
    {
      actn->setEnabled(rowCount > 0);
      QObject::connect(actn, &QAction::triggered, [order, section, sfmodel, header](bool) {
        sfmodel->sort(section, order);
        header->setSortIndicatorShown(true);
        // this needs to be inverted to match the icons we use in other places
        // like SpreadSheetView.
        header->setSortIndicator(
          section, order == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
      });
    }

    if (auto actn = menu.addAction("Clear sorting"))
    {
      actn->setEnabled(sfmodel->sortColumn() != -1);
      QObject::connect(actn, &QAction::triggered, [sfmodel, header](bool) {
        sfmodel->sort(-1);
        header->setSortIndicatorShown(false);
      });
    }
  }
  menu.exec(this->TreeView->mapToGlobal(pos));
}
