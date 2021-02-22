/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidget.cxx

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

#include "pqTreeWidget.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"

#include <QApplication>
#include <QHeaderView>
#include <QLayout>
#include <QPainter>
#include <QStyle>

#include "pqTimer.h"

// enum for different pixmap types
enum pqTreeWidgetPixmap
{
  pqCheck = 0,
  pqPartialCheck = 1,
  pqUnCheck = 2,

  // All active states in lower half
  pqCheck_Active = 3,
  pqPartialCheck_Active = 4,
  pqUnCheck_Active = 5,

  pqMaxCheck = 6
};

//-----------------------------------------------------------------------------
// array of style corresponding with the pqTreeWidgetPixmap enum
static const QStyle::State pqTreeWidgetPixmapStyle[] = { QStyle::State_On | QStyle::State_Enabled,
  QStyle::State_NoChange | QStyle::State_Enabled, QStyle::State_Off | QStyle::State_Enabled,
  QStyle::State_On | QStyle::State_Enabled | QStyle::State_Active,
  QStyle::State_NoChange | QStyle::State_Enabled | QStyle::State_Active,
  QStyle::State_Off | QStyle::State_Enabled | QStyle::State_Active };

//-----------------------------------------------------------------------------
QPixmap pqTreeWidget::pixmap(Qt::CheckState cs, bool active)
{
  int offset = active ? pqMaxCheck / 2 : 0;
  switch (cs)
  {
    case Qt::Checked:
      return *this->CheckPixmaps[offset + pqCheck];
    case Qt::Unchecked:
      return *this->CheckPixmaps[offset + pqUnCheck];
    case Qt::PartiallyChecked:
      return *this->CheckPixmaps[offset + pqPartialCheck];
  }
  return QPixmap();
}

//-----------------------------------------------------------------------------
pqTreeWidget::pqTreeWidget(QWidget* p)
  : QTreeWidget(p)
  , MaximumRowCountBeforeScrolling(10)
{
  QStyleOptionButton option;
  QRect r = this->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, this);
  option.rect = QRect(QPoint(0, 0), r.size());

  this->CheckPixmaps = new QPixmap*[6];
  for (int i = 0; i < pqMaxCheck; i++)
  {
    this->CheckPixmaps[i] = new QPixmap(r.size());
    this->CheckPixmaps[i]->fill(QColor(0, 0, 0, 0));
    QPainter painter(this->CheckPixmaps[i]);
    option.state = pqTreeWidgetPixmapStyle[i];

    this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, this);
  }

  QObject::connect(
    this->header(), SIGNAL(sectionClicked(int)), this, SLOT(doToggle(int)), Qt::QueuedConnection);

  this->header()->setSectionsClickable(true);

  QObject::connect(
    this->model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(updateCheckState()));
  QObject::connect(
    this->model(), SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(updateCheckState()));

  QObject::connect(
    this->model(), SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(invalidateLayout()));
  QObject::connect(
    this->model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(invalidateLayout()));
  QObject::connect(this->model(), SIGNAL(modelReset()), this, SLOT(invalidateLayout()));

  this->Timer = new pqTimer(this);
  this->Timer->setSingleShot(true);
  this->Timer->setInterval(10);
  QObject::connect(this->Timer, SIGNAL(timeout()), this, SLOT(updateCheckStateInternal()));

  // better handle scrolling in panels with nested scrollbars.
  this->setFocusPolicy(Qt::StrongFocus);
}

//-----------------------------------------------------------------------------
pqTreeWidget::~pqTreeWidget()
{
  delete this->Timer;
  for (int i = 0; i < pqMaxCheck; i++)
  {
    delete this->CheckPixmaps[i];
  }
  delete[] this->CheckPixmaps;
}

//-----------------------------------------------------------------------------
bool pqTreeWidget::event(QEvent* e)
{
  if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut)
  {
    bool convert = false;
    int cs = this->headerItem()->data(0, Qt::CheckStateRole).toInt(&convert);
    if (convert)
    {
      bool active = e->type() == QEvent::FocusIn;
      this->headerItem()->setData(
        0, Qt::DecorationRole, pixmap(static_cast<Qt::CheckState>(cs), active));
    }
  }

  return Superclass::event(e);
}

//-----------------------------------------------------------------------------
void pqTreeWidget::wheelEvent(QWheelEvent* evt)
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
void pqTreeWidget::updateCheckState()
{
  this->Timer->start();
  // updateCheckStateInternal needs to call rowIndex() which forces the tree to
  // sort. Hence when multiple items are being added/updated the tree is sorted
  // after every insert. To avoid that we use this timer.
}

//-----------------------------------------------------------------------------
void pqTreeWidget::updateCheckStateInternal()
{
  Qt::CheckState newState = Qt::Checked;
  int numChecked = 0;
  int numPartial = 0;
  int numUnchecked = 0;
  QAbstractItemModel* m = this->model();
  int numRows = m->rowCount(QModelIndex());
  for (int i = 0; i < numRows; i++)
  {
    QModelIndex idx = m->index(i, 0);
    bool convert = 0;
    int v = m->data(idx, Qt::CheckStateRole).toInt(&convert);
    if (convert)
    {
      if (v == Qt::Checked)
      {
        numChecked++;
      }
      else if (v == Qt::PartiallyChecked)
      {
        numPartial++;
      }
      else
      {
        numUnchecked++;
      }
    }
  }

  // if there are no check boxes at all
  if (0 == (numUnchecked + numPartial + numChecked))
  {
    return;
  }

  if (numChecked != numRows)
  {
    newState = ((numChecked == 0) && (numPartial == 0)) ? Qt::Unchecked : Qt::PartiallyChecked;
  }

  this->headerItem()->setCheckState(0, newState);
  this->headerItem()->setData(0, Qt::DecorationRole, pixmap(newState, this->hasFocus()));
}

//-----------------------------------------------------------------------------
void pqTreeWidget::allOn()
{
  QTreeWidgetItem* item;
  int i, end = this->topLevelItemCount();
  for (i = 0; i < end; i++)
  {
    item = this->topLevelItem(i);
    item->setCheckState(0, Qt::Checked);
  }
}

//-----------------------------------------------------------------------------
void pqTreeWidget::allOff()
{
  QTreeWidgetItem* item;
  int i, end = this->topLevelItemCount();
  for (i = 0; i < end; i++)
  {
    item = this->topLevelItem(i);
    item->setCheckState(0, Qt::Unchecked);
  }
}

//-----------------------------------------------------------------------------
void pqTreeWidget::doToggle(int column)
{
  if (column == 0)
  {
    bool convert = false;
    int cs = this->headerItem()->data(0, Qt::CheckStateRole).toInt(&convert);
    if (convert)
    {
      if (cs == Qt::Checked)
      {
        this->allOff();
      }
      else
      {
        // both unchecked and partial checked go here
        this->allOn();
      }
    }
  }
}

//-----------------------------------------------------------------------------
int pqTreeWidget::itemCount(QTreeWidgetItem* item) const
{
  int maxItemHint = this->MaximumRowCountBeforeScrolling;
  int numItems = item ? item->childCount() : this->topLevelItemCount();
  int count = numItems;
  for (int cc = 0; cc < numItems; cc++)
  {
    QTreeWidgetItem* childItem = item ? item->child(cc) : this->topLevelItem(cc);
    count += this->itemCount(childItem);
    if (count > maxItemHint)
    {
      // cut short traversal of the tree if we've reached the max size.
      return maxItemHint;
    }
  }

  return count;
}

//-----------------------------------------------------------------------------
QSize pqTreeWidget::sizeHint() const
{
  // lets show X items before we get a scrollbar
  // probably want to make this a member variable
  // that a caller has access to
  int maxItemHint = this->MaximumRowCountBeforeScrolling;
  // for no items, let's give a space of X pixels
  int minItemHeight = 20;

  int num = this->itemCount(nullptr) + 1; /* extra room for scroll bar */
  num = qMin(num, maxItemHint);

  int pix = minItemHeight;

  if (num)
  {
    pix = qMax(pix, this->sizeHintForRow(0) * num);
  }

  QMargins margin = this->contentsMargins();
  int h = pix + margin.top() + margin.bottom() + this->header()->frameSize().height();
  return QSize(156, h);
}

//-----------------------------------------------------------------------------
QSize pqTreeWidget::minimumSizeHint() const
{
  return this->sizeHint();
}

//-----------------------------------------------------------------------------
void pqTreeWidget::invalidateLayout()
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
void pqTreeWidget::setMaximumRowCountBeforeScrolling(vtkSMPropertyGroup* smpropertygroup)
{
  this->setMaximumRowCountBeforeScrolling(smpropertygroup->GetHints());
}

//-----------------------------------------------------------------------------
void pqTreeWidget::setMaximumRowCountBeforeScrolling(vtkSMProperty* smproperty)
{
  this->setMaximumRowCountBeforeScrolling(smproperty->GetHints());
}

//-----------------------------------------------------------------------------
void pqTreeWidget::setMaximumRowCountBeforeScrolling(vtkPVXMLElement* hints)
{
  if (hints)
  {
    vtkPVXMLElement* element = hints->FindNestedElementByName("WidgetHeight");
    if (element)
    {
      const char* rowCount = element->GetAttribute("number_of_rows");
      if (rowCount)
      {
        this->setMaximumRowCountBeforeScrolling(QString(rowCount).toInt());
      }
    }
  }
}

//-----------------------------------------------------------------------------
QModelIndex pqTreeWidget::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
  QModelIndex suggestedIndex = this->Superclass::moveCursor(cursorAction, modifiers);

  int max_rows = this->topLevelItemCount();
  int max_colums = this->columnCount();
  QTreeWidgetItem* curItem = this->currentItem();
  int cur_col = this->currentColumn();
  if (!curItem || cur_col < 0 || cur_col >= max_colums)
  {
    return suggestedIndex;
  }

  int cur_row = this->indexOfTopLevelItem(curItem);
  if (cursorAction == QAbstractItemView::MoveNext && modifiers == Qt::NoModifier)
  {
    int next_column = cur_col + 1;
    while (next_column < max_colums && this->isColumnHidden(next_column))
    {
      // skip hidden columns.
      next_column++;
    }
    if (next_column < max_colums)
    {
      return this->indexFromItem(curItem, next_column);
    }
    else if ((cur_row + 1) == max_rows)
    {
      // User is at last row, we need to add a new row before moving to that
      // row.
      Q_EMIT this->navigatedPastEnd();
      // if the table grows, the index may change.
      suggestedIndex = this->Superclass::moveCursor(cursorAction, modifiers);
    }
    // otherwise default behavior takes it to the first column in the next
    // row, which is what is expected.
  }
  else if (cursorAction == QAbstractItemView::MovePrevious && modifiers == Qt::NoModifier)
  {
    int prev_column = cur_col - 1;
    while (prev_column >= 0 && this->isColumnHidden(prev_column))
    {
      // skip hidden columns.
      prev_column--;
    }
    if (prev_column >= 0)
    {
      return this->indexFromItem(curItem, prev_column);
    }
    else
    {
      // we need to go to the last column in the previous row.
      if (cur_row > 0)
      {
        prev_column = max_colums - 1;
        while (prev_column >= 0 && this->isColumnHidden(prev_column))
        {
          // skip hidden columns.
          prev_column--;
        }
        if (prev_column >= 0)
        {
          return this->indexFromItem(this->topLevelItem(cur_row - 1), max_colums - 1);
        }
      }
    }
  }

  return suggestedIndex;
}
