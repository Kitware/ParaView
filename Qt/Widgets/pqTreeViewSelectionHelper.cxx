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

// Server Manager Includes.

// Qt Includes.
#include <QItemSelection>
#include <QMenu>
#include <QtDebug>

// ParaView Includes.

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::pqTreeViewSelectionHelper(QTreeView* tree)
  : Superclass(tree)
{
  this->TreeView = tree;
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(tree, SIGNAL(clicked(QModelIndex)), this, SLOT(onClicked(QModelIndex)));
  QObject::connect(tree, SIGNAL(pressed(QModelIndex)), this, SLOT(onPressed(QModelIndex)));

  QObject::connect(tree, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(showContextMenu(const QPoint&)));

  QObject::connect(tree->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(saveSelection()));
}

//-----------------------------------------------------------------------------
pqTreeViewSelectionHelper::~pqTreeViewSelectionHelper()
{
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::saveSelection()
{
  this->PrevSelection = this->CurrentSelection;
  this->CurrentSelection = this->TreeView->selectionModel()->selection();
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::onPressed(QModelIndex idx)
{
  //  qDebug() << "onItemPressed"
  //  << this->TreeWidget->selectionModel()->selectedIndexes().size();

  this->PressState = -1;

  Qt::ItemFlags flags = this->TreeView->model()->flags(idx);

  if ((flags & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
  {
    this->PressState = this->TreeView->model()->data(idx, Qt::CheckStateRole).toInt();
  }
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::onClicked(QModelIndex idx)
{
  //  qDebug() << "onItemClicked"
  //  << this->TreeWidget->selectionModel()->selectedIndexes().size();
  if (this->PrevSelection.contains(idx) && this->PressState != -1)
  {
    Qt::CheckState state =
      static_cast<Qt::CheckState>(this->TreeView->model()->data(idx, Qt::CheckStateRole).toInt());
    if (state != this->PressState)
    {
      // Change all checkable items in the this->Selection to match the new
      // check state.
      this->setSelectedItemsCheckState(state);
    }
  }
  this->saveSelection();
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::setSelectedItemsCheckState(Qt::CheckState state)
{
  // Change all checkable items in the this->Selection to match the new
  // check state.
  foreach (QModelIndex idx, this->PrevSelection.indexes())
  {
    Qt::ItemFlags flags = this->TreeView->model()->flags(idx);

    if ((flags & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
    {
      this->TreeView->model()->setData(idx, state, Qt::CheckStateRole);
    }
  }

  this->TreeView->selectionModel()->select(
    this->PrevSelection, QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqTreeViewSelectionHelper::showContextMenu(const QPoint& pos)
{
  if (this->TreeView->selectionModel()->selectedIndexes().size() > 0)
  {
    QMenu menu;
    menu.setObjectName("TreeViewCheckMenu");
    QAction* check = new QAction("Check", &menu);
    QAction* uncheck = new QAction("Uncheck", &menu);
    QAction* sort = new QAction("Sort by Name", &menu);
    QAction* unsort = new QAction("Sort by Block Index", &menu);
    menu.addAction(check);
    menu.addAction(uncheck);
    menu.addAction(sort);
    menu.addAction(unsort);
    if (this->TreeView->isSortingEnabled())
    {
      sort->setEnabled(false);
    }
    else
    {
      unsort->setEnabled(false);
    }
    QAction* result = menu.exec(this->TreeView->mapToGlobal(pos));
    if (result == check)
    {
      this->setSelectedItemsCheckState(Qt::Checked);
    }
    else if (result == uncheck)
    {
      this->setSelectedItemsCheckState(Qt::Unchecked);
    }
    else if (result == sort)
    {
      this->TreeView->setSortingEnabled(true);
      this->TreeView->sortByColumn(0, Qt::AscendingOrder);
    }
    else if (result == unsort)
    {
      this->TreeView->sortByColumn(-1);
      this->TreeView->setSortingEnabled(false);
    }
  }
}
