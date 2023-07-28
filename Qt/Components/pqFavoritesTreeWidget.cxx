// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFavoritesTreeWidget.h"

#include <QTreeWidgetItem>

//----------------------------------------------------------------------------
pqFavoritesTreeWidget::pqFavoritesTreeWidget(QWidget* p)
  : Superclass(p)
{
}

//----------------------------------------------------------------------------
void pqFavoritesTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
  this->UnfoldedDraggedCategories.clear();
  for (auto* item : this->selectedItems())
  {
    if (item->isExpanded())
    {
      this->UnfoldedDraggedCategories.insert(item);
    }
  }

  Superclass::dragEnterEvent(event);
}

//----------------------------------------------------------------------------
void pqFavoritesTreeWidget::dropEvent(QDropEvent* event)
{
  Superclass::dropEvent(event);
  // Superclass automatically fold dropped item, so manually
  // keep expanded categories that were expanded.
  for (auto* item : this->UnfoldedDraggedCategories)
  {
    item->setExpanded(true);
  }
  this->resizeColumnToContents(0);
}
