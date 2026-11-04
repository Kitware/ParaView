// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTableView.h"

#include <algorithm>
#include <cassert>

#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLayout>
#include <QScrollBar>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqTableView::pqTableView(QWidget* parentObject)
  : Superclass(parentObject)
  , MaximumRowCountBeforeScrolling(0)
  , MinimumRowCount(1)
  , Padding(1)
  , ScrollPadding(0)
{
  // listen for show/hide events on the horizontal scroll bar.
  this->horizontalScrollBar()->installEventFilter(this);

  // better handle scrolling in panels with nested scrollbars.
  this->setFocusPolicy(Qt::StrongFocus);
}

//-----------------------------------------------------------------------------
pqTableView::~pqTableView() = default;

//-----------------------------------------------------------------------------
bool pqTableView::eventFilter(QObject* object, QEvent* e)
{
  if (this->MaximumRowCountBeforeScrolling > 0)
  {
    if (object == this->horizontalScrollBar())
    {
      if (e->type() == QEvent::Show && this->ScrollPadding == 0)
      {
        this->ScrollPadding = this->horizontalScrollBar()->height();
        this->invalidateLayout();
      }
      else if (e->type() == QEvent::Hide && this->ScrollPadding != 0)
      {
        this->ScrollPadding = 0;
        this->invalidateLayout();
      }
    }
  }
  return this->Superclass::eventFilter(object, e);
}

//-----------------------------------------------------------------------------
void pqTableView::wheelEvent(QWheelEvent* evt)
{
  // don't handle wheel events unless widget had focus.
  // this improves scrolling when scrollable widgets are nested
  // together with setFocusPolicy(Qt::StrongFocus).
  if (this->hasFocus())
  {
    this->Superclass::wheelEvent(evt);
  }
  else
  {
    // Explicitly ignore the event so that it propagates to the parent widget.
    evt->ignore();
  }
}

//-----------------------------------------------------------------------------
void pqTableView::keyPressEvent(QKeyEvent* evt)
{
  if (evt->matches(QKeySequence::Copy))
  {
    int previousRow = 0;
    auto selection = this->selectedIndexes();
    QStringList row;
    QStringList table;
    for (const auto& index : selection)
    {
      if (index.row() > previousRow && !row.isEmpty())
      {
        table << row.join('\t');
        row.clear();
      }
      previousRow = index.row();
      row << index.data().toString();
    }
    if (!row.empty())
    {
      table << row.join('\t');
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(table.join("\r\n"));
  }
  else
  {
    Superclass::keyPressEvent(evt);
  }
}

//-----------------------------------------------------------------------------
void pqTableView::setModel(QAbstractItemModel* mdl)
{
  QAbstractItemModel* curModel = this->model();
  if (curModel)
  {
    // disconnect all connections on model that connect to invalidateLayout();
    curModel->disconnect(/*receiver=*/this, /*method=*/SLOT(invalidateLayout()));
  }
  this->Superclass::setModel(mdl);
  if (mdl)
  {
    this->connect(
      mdl, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(invalidateLayout()));
    this->connect(mdl, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(invalidateLayout()));
    this->connect(mdl, SIGNAL(modelReset()), SLOT(invalidateLayout()));
  }
  this->invalidateLayout();
}

//-----------------------------------------------------------------------------
void pqTableView::setRootIndex(const QModelIndex& idx)
{
  this->Superclass::setRootIndex(idx);
  this->invalidateLayout();
}

//-----------------------------------------------------------------------------
void pqTableView::invalidateLayout()
{
  if (this->MaximumRowCountBeforeScrolling == 0)
  {
    return;
  }

  // sizeHint is dynamic, so we need to invalidate parent layouts
  // when items are added or removed
  for (QWidget* w = this->parentWidget(); w && w->layout(); w = w->parentWidget())
  {
    w->layout()->invalidate();
  }

  // invalidate() is not enough, we need to reset the cache of the
  // QWidgetItemV2, so sizeHint() could be recomputed.
  this->updateGeometry();
}

//-----------------------------------------------------------------------------
QSize pqTableView::minimumSizeHint() const
{
  return this->MaximumRowCountBeforeScrolling > 0
    ? QSize(this->Superclass::minimumSizeHint().width(), this->sizeHint().height())
    : this->Superclass::minimumSizeHint();
}

//-----------------------------------------------------------------------------
QSize pqTableView::sizeHint() const
{
  if (this->MaximumRowCountBeforeScrolling <= 0)
  {
    return this->Superclass::sizeHint();
  }

  int rows = this->model() ? this->model()->rowCount(this->rootIndex()) : 0;

  if (rows < this->MinimumRowCount)
  {
    rows = qMax(0, this->MinimumRowCount);
  }
  rows += qMax(0, this->Padding);

  rows = std::min(rows, this->MaximumRowCountBeforeScrolling);

  // rows can't be negative.
  assert(rows >= 0);

  int pixelsPerRow = this->sizeHintForRow(0);
  pixelsPerRow = qMax(pixelsPerRow, this->verticalHeader()->defaultSectionSize());
  pixelsPerRow = qMax(pixelsPerRow, this->verticalHeader()->minimumSectionSize());

  int pixels = rows * pixelsPerRow;

  QMargins margin = this->contentsMargins();
  int viewHeight = pixels + margin.top() + margin.bottom();
  if (this->horizontalHeader()->isVisible())
  {
    viewHeight += this->horizontalHeader()->frameSize().height();
  }
  viewHeight += this->ScrollPadding;
  return QSize(this->Superclass::sizeHint().width(), viewHeight);
}
