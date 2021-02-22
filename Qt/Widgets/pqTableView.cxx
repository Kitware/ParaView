/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqTableView.h"

#include "cassert"

#include <QAbstractItemModel>
#include <QEvent>
#include <QHeaderView>
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

  if (rows > this->MaximumRowCountBeforeScrolling)
  {
    rows = this->MaximumRowCountBeforeScrolling;
  }

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
