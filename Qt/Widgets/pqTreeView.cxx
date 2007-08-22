/*=========================================================================

   Program: ParaView
   Module:    pqTreeView.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqTreeView.cxx
/// \date 8/20/2007

#include "pqTreeView.h"

#include "pqCheckableHeaderView.h"
#include <QLayout>


pqTreeView::pqTreeView(QWidget *widgetParent)
  : QTreeView(widgetParent)
{
  // Change the default header view to a checkable one.
  pqCheckableHeaderView *checkable = new pqCheckableHeaderView(Qt::Horizontal);
  this->setHeader(checkable);
  this->installEventFilter(checkable);
  checkable->setClickable(true);
}

void pqTreeView::setModel(QAbstractItemModel *newModel)
{
  QAbstractItemModel *current = this->model();
  if(current)
    {
    this->disconnect(current, 0, this, 0);
    }

  QTreeView::setModel(newModel);
  if(newModel)
    {
    this->connect(
        newModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        this, SLOT(invalidateLayout()));
    this->connect(newModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
        this, SLOT(invalidateLayout()));
    this->connect(newModel, SIGNAL(modelReset()),
        this, SLOT(invalidateLayout()));
    }

  this->invalidateLayout();
}

void pqTreeView::setRootIndex(const QModelIndex &index)
{
  QTreeView::setRootIndex(index);
  this->invalidateLayout();
}

QSize pqTreeView::sizeHint() const
{
  // lets show X items before we get a scrollbar
  // probably want to make this a member variable
  // that a caller has access to
  int maxItemHint = 10;
  // for no items, let's give a space of X pixels
  int minItemHeight = 20;

  int num = 0;
  QAbstractItemModel *current = this->model();
  if(current)
    {
    num = current->rowCount(this->rootIndex());
    }

  num = qMin(num, maxItemHint);
  
  int pix = minItemHeight;

  if(num)
    {
    pix = qMax(pix, this->sizeHintForRow(0) * num);
    }

  int margin[4];
  this->getContentsMargins(margin, margin+1, margin+2, margin+3);
  int h = pix + margin[1] + margin[3] + this->header()->frameSize().height();
  return QSize(156, h);
}

QSize pqTreeView::minimumSizeHint() const
{
  return this->sizeHint();
}

void pqTreeView::invalidateLayout()
{
  // sizeHint is dynamic, so we need to invalidate parent layouts
  // when items are added or removed
  for(QWidget* w = this->parentWidget();
      w && w->layout();
      w = w->parentWidget())
    {
    w->layout()->invalidate();
    }
}


