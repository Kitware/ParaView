/*=========================================================================

   Program: ParaView
   Module:  pqTreeView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqTreeView.h"

#include "pqCheckableHeaderView.h"
#include "pqHeaderView.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QLayout>
#include <QMimeData>
#include <QPointer>
#include <QScrollBar>

namespace
{
int pqCountItems(QAbstractItemModel* model, const QModelIndex& idx, int max)
{
  if (!model)
  {
    return 0;
  }
  int numItems = model->rowCount(idx);
  int count = numItems;
  for (int cc = 0; cc < numItems; ++cc)
  {
    if (count >= max)
    {
      return count;
    }
    count += pqCountItems(model, model->index(cc, 0, idx), max - numItems);
  }
  return count;
}
}

//-----------------------------------------------------------------------------
pqTreeView::pqTreeView(QWidget* widgetParent, bool use_pqHeaderView)
  : QTreeView(widgetParent)
  , ScrollPadding(0)
  , MaximumRowCountBeforeScrolling(10)
{
  this->ScrollPadding = 0;

  // Change the default header view to a checkable one.
  QPointer<QHeaderView> oldheader = this->header();
  if (use_pqHeaderView)
  {
    pqHeaderView* myheader = new pqHeaderView(Qt::Horizontal, this);
    myheader->setToggleCheckStateOnSectionClick(true);
    myheader->setStretchLastSection(oldheader->stretchLastSection());
    this->setHeader(myheader);
  }
  else
  {
    // in the long run, I'd like to get rid of the pqCheckableHeaderView
    // entirely.
    pqCheckableHeaderView* checkable = new pqCheckableHeaderView(Qt::Horizontal, this);
    checkable->setStretchLastSection(oldheader->stretchLastSection());
    this->setHeader(checkable);
    this->installEventFilter(checkable);
    checkable->setSectionsClickable(true);
  }
  delete oldheader;

  // Listen for the show/hide events of the horizontal scroll bar.
  this->horizontalScrollBar()->installEventFilter(this);

  // better handle scrolling in panels with nested scrollbars.
  this->setFocusPolicy(Qt::StrongFocus);
}

//-----------------------------------------------------------------------------
bool pqTreeView::eventFilter(QObject* object, QEvent* e)
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

  return QTreeView::eventFilter(object, e);
}

//-----------------------------------------------------------------------------
void pqTreeView::wheelEvent(QWheelEvent* evt)
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
void pqTreeView::setModel(QAbstractItemModel* newModel)
{
  QAbstractItemModel* current = this->model();
  if (current)
  {
    this->disconnect(current, 0, this, 0);
  }

  QTreeView::setModel(newModel);
  if (newModel)
  {
    this->connect(
      newModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(invalidateLayout()));
    this->connect(
      newModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(invalidateLayout()));
    this->connect(newModel, SIGNAL(modelReset()), this, SLOT(invalidateLayout()));
  }

  this->invalidateLayout();
}

//-----------------------------------------------------------------------------
void pqTreeView::setRootIndex(const QModelIndex& index)
{
  QTreeView::setRootIndex(index);
  this->invalidateLayout();
}

//-----------------------------------------------------------------------------
QSize pqTreeView::sizeHint() const
{
  // lets show X items before we get a scrollbar
  // probably want to make this a member variable
  // that a caller has access to
  int maxItemHint = this->MaximumRowCountBeforeScrolling;
  // for no items, let's give a space of X pixels
  int minItemHeight = 20;
  // add padding for the scrollbar
  int extra = this->ScrollPadding;
  int num = pqCountItems(this->model(), this->rootIndex(), maxItemHint);
  if (num >= maxItemHint)
  {
    extra = 0;
    num = maxItemHint;
  }

  int pix = minItemHeight;

  if (num)
  {
    num++; // leave an extra row padding.
           // the widget ends up appearing too crowded otherwise.
    pix = qMax(pix, this->sizeHintForRow(0) * num);
  }

  int margin[4];
  this->getContentsMargins(margin, margin + 1, margin + 2, margin + 3);
  int h = pix + margin[1] + margin[3] + this->header()->frameSize().height();
  return QSize(this->Superclass::sizeHint().width(), h + extra);
}

//-----------------------------------------------------------------------------
QSize pqTreeView::minimumSizeHint() const
{
  return this->sizeHint();
}

//-----------------------------------------------------------------------------
void pqTreeView::invalidateLayout()
{
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
QItemSelectionModel::SelectionFlags pqTreeView::selectionCommand(
  const QModelIndex& idx, const QEvent* evt) const
{
  if (evt != nullptr && evt->type() == QEvent::MouseButtonRelease && idx.isValid())
  {
    return QItemSelectionModel::NoUpdate;
  }
  return this->Superclass::selectionCommand(idx, evt);
}

//-----------------------------------------------------------------------------
bool pqTreeView::edit(const QModelIndex& idx, EditTrigger trigger, QEvent* evt)
{
  const bool edited = this->Superclass::edit(idx, trigger, evt);

  if (trigger == SelectedClicked && edited)
  {
    auto amodel = this->model();
    auto itemFlags = amodel->flags(idx);
    if ((itemFlags & Qt::ItemIsUserCheckable) && (itemFlags & Qt::ItemIsEnabled))
    {
      const auto itemCheckState = amodel->data(idx, Qt::CheckStateRole);

      // user clicked on a selected item, update all other selected items check
      // state, if possible, to match the clicked item's check state.
      for (const QModelIndex& otherIdx : this->selectedIndexes())
      {
        if (otherIdx != idx)
        {
          auto otherItemFlags = amodel->flags(otherIdx);
          if ((otherItemFlags & Qt::ItemIsUserCheckable) && (otherItemFlags & Qt::ItemIsEnabled))
          {
            // several models may report flags incorrectly. Hence check for valid
            // checkstate data as well,  before setting check state.
            const QVariant idata = amodel->data(otherIdx, Qt::CheckStateRole);
            if (idata.isValid() && idata != itemCheckState)
            {
              amodel->setData(otherIdx, itemCheckState, Qt::CheckStateRole);
            }
          }
        }
      }
    }
  }
  return edited;
}
