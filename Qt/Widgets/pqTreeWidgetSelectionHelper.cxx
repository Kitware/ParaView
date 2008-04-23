/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetSelectionHelper.cxx

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
#include "pqTreeWidgetSelectionHelper.h"

// Server Manager Includes.

// Qt Includes.
#include <QTreeWidget>
#include <QtDebug>
#include <QMenu>
// ParaView Includes.

//-----------------------------------------------------------------------------
pqTreeWidgetSelectionHelper::pqTreeWidgetSelectionHelper(QTreeWidget* tree):
  Superclass(tree)
{
  this->TreeWidget = tree;
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
    this, SLOT(onItemClicked(QTreeWidgetItem*, int)));
  QObject::connect(tree, SIGNAL(itemPressed(QTreeWidgetItem*, int)),
    this, SLOT(onItemPressed(QTreeWidgetItem*, int)));
  QObject::connect(tree, SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(showContextMenu(const QPoint&)));
}

//-----------------------------------------------------------------------------
pqTreeWidgetSelectionHelper::~pqTreeWidgetSelectionHelper()
{
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::onItemPressed(QTreeWidgetItem* item, int )
{
//  qDebug() << "onItemPressed" 
//  << this->TreeWidget->selectionModel()->selectedIndexes().size();

  this->PressState = -1;
  if ((item->flags() & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
    {
    this->PressState = item->checkState(0);
    this->Selection = this->TreeWidget->selectionModel()->selection();
    }
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::onItemClicked(QTreeWidgetItem* item, int)
{
  //  qDebug() << "onItemClicked" 
  //  << this->TreeWidget->selectionModel()->selectedIndexes().size();
  if (this->PressState != -1)
    {
    Qt::CheckState state = item->checkState(0);
    if (state != this->PressState)
      {
      // Change all checkable items in the this->Selection to match the new
      // check state.
      this->setSelectedItemsCheckState(state);
      }
    }
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::setSelectedItemsCheckState(Qt::CheckState state)
{
  // Change all checkable items in the this->Selection to match the new
  // check state.
  this->TreeWidget->selectionModel()->select(this->Selection, 
    QItemSelectionModel::ClearAndSelect);

  QList<QTreeWidgetItem*> items = this->TreeWidget->selectedItems();
  foreach (QTreeWidgetItem* curitem, items)
    {
    if ((curitem->flags() & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
      {
      curitem->setCheckState(/*column*/0, state);
      }
    }
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::showContextMenu(const QPoint &pos)
{
  if (this->TreeWidget->selectionModel()->selectedIndexes().size() > 0)
    {
    QMenu menu;
    menu.setObjectName("TreeWidgetCheckMenu");
    QAction* check = new QAction("Check", &menu);
    QAction* uncheck = new QAction("Uncheck", &menu);
    menu.addAction(check);
    menu.addAction(uncheck);
    QAction* result = menu.exec(this->TreeWidget->mapToGlobal(pos));
    if (result == check)
      {
      this->setSelectedItemsCheckState(Qt::Checked);
      }
    else if (result == uncheck)
      {
      this->setSelectedItemsCheckState(Qt::Unchecked);
      }
    }
}
