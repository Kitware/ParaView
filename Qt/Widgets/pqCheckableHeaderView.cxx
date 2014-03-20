/*=========================================================================

   Program: ParaView
   Module:    pqCheckableHeaderView.cxx

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

/// \file pqCheckableHeaderView.cxx
/// \date 03/04/2014

#include "pqCheckableHeaderView.h"

#include <QMouseEvent>
#include <QPainter>

//----------------------------------------------------------------------------
class pqCheckableHeaderViewInternal
{
public:
  pqCheckableHeaderViewInternal()
    {
    this->Style = 0;
    this->forceCheck = false;
    }
  ~pqCheckableHeaderViewInternal()
    {
    }

  /// Returns the placeholder rect for a checkbox in the section header
  QRect checkBoxRect(const QRect &sourceRect,
    const QAbstractItemView *view) const;

  /// Draw the checkbox control
  /// Based on two scenarios:
  /// 1. If any of the item checkboxes are changed, the section header checkbox
  /// needs to be updated.
  /// 2. If the section header checkbox is clicked and its state needs to be
  /// toggled.
  /// It returns the new checkbox state
  /// \arg checkState: State to force the header checkbox to. Used when clicked on
  /// the checkbox.
  QVariant drawCheckboxControl(QPainter *painter, const QRect &rect,
    int numItemsChecked, int totalItems, QVariant checkState,
    const QAbstractItemView *view);

  QStyle *Style;

  /// Map of (section,checkable) values indicating whether
  /// the section is checkable
  QHash<int, bool> isCheckable;

  /// Map of (section,checkstate) values indicating whether
  /// the section checkbox is checked/partially checked/unchecked
  QHash<int, QVariant> checkState;

  /// Boolean to force check/uncheck state of header checkbox
  bool forceCheck;
};

//----------------------------------------------------------------------------
QRect pqCheckableHeaderViewInternal::checkBoxRect(
  const QRect &sourceRect, const QAbstractItemView *view) const
{
  QStyleOptionButton checkBoxStyleOption;
  QRect checkBoxRect = this->Style->subElementRect(
    QStyle::SE_CheckBoxIndicator,
    &checkBoxStyleOption);
  int buttonMargin = this->Style->pixelMetric(
    QStyle::PM_ButtonMargin, NULL, view);
  QPoint checkBoxPoint(
    sourceRect.x() + buttonMargin,
    sourceRect.y() + buttonMargin);
  return QRect(checkBoxPoint, checkBoxRect.size());
}

//----------------------------------------------------------------------------
QVariant pqCheckableHeaderViewInternal::drawCheckboxControl(
  QPainter *painter,
  const QRect &rect,
  int numItemsChecked,
  int totalItems,
  QVariant checkState,
  const QAbstractItemView *view)
{
  QVariant isChecked;

  QStyleOptionButton option;
  option.rect = this->checkBoxRect(rect, view);
  option.state |= QStyle::State_Enabled;

  if (this->forceCheck)
    {
    if (checkState.toInt() == Qt::Unchecked)
      {
      option.state |= QStyle::State_Off;
      }
    else
      {
      option.state |= QStyle::State_On;
      }
    isChecked = checkState;
    }
  else
    {
      if (numItemsChecked == 0)
      {
      option.state |= QStyle::State_Off;
      isChecked = QVariant(Qt::Unchecked);
      }
    else if (numItemsChecked < totalItems)
      {
      option.state |= QStyle::State_NoChange;
      isChecked = QVariant(Qt::PartiallyChecked);
      }
    else if (numItemsChecked == totalItems)
      {
      option.state |= QStyle::State_On;
      isChecked = QVariant(Qt::Checked);
      }
    else
      {
      // Invalid
      option.state |= QStyle::State_None;
      }
    }
  this->Style->drawControl(QStyle::CE_CheckBox, &option, painter);

  // Reset the forceCheck boolean once the checkbox is drawn
  this->forceCheck = false;

  return isChecked;
}

//----------------------------------------------------------------------------
pqCheckableHeaderView::pqCheckableHeaderView(Qt::Orientation orientation,
  QWidget *parent) :
  QHeaderView(orientation, parent)
{
  this->Internal = new pqCheckableHeaderViewInternal();
  this->Internal->Style = this->style();
#if QT_VERSION >= 0x050000
  setSectionsClickable(true);
#else
  setClickable(true);
#endif
  QObject::connect(this, SIGNAL(checkStateChanged(int)),
    this, SLOT(updateSection(int)));
}

//----------------------------------------------------------------------------
pqCheckableHeaderView::~pqCheckableHeaderView()
{
  if (this->Internal)
    {
    delete this->Internal;
    }
}

//----------------------------------------------------------------------------
void pqCheckableHeaderView::paintSection(QPainter *painter,
  const QRect &rect, int logicalIndex) const
{
  painter->save();
  QHeaderView::paintSection(painter, rect, logicalIndex);
  painter->restore();

  QAbstractItemModel *model = this->model();
  if (!model)
    {
    return;
    }

  // Total number of top-level items in this section
  int totalItems = this->orientation() == Qt::Horizontal ?
    model->rowCount() : model->columnCount();

  bool checkable = false;
  int numItemsChecked = 0;

  if (this->orientation() == Qt::Horizontal)
    {
    for (int i = 0; i < totalItems; i++)
      {
      QModelIndex idx;
      if (this->orientation() == Qt::Horizontal)
        {
        // If headerview is horizontal, the sections are columns and iterate
        // over rows
        idx = model->index(i, logicalIndex);
        }
      else
        {
        // If headerview is vertical, the sections are rows and iterate over
        // columns
        idx = model->index(logicalIndex, i);
        }
      Qt::ItemFlags f = this->model()->flags(idx);
      if ((f & Qt::ItemIsUserCheckable) != 0)
        {
        checkable = true;
        QVariant value = this->model()->data(idx, Qt::CheckStateRole);
        if (value.toInt() != Qt::Unchecked)
          {
          numItemsChecked++;
          }
        }
      }
    }
  if (checkable)
    {
    QVariant checkstate;
    if(this->Internal->checkState.contains(logicalIndex))
      {
      checkstate = this->Internal->checkState[logicalIndex];
      }
    QVariant newCheckstate = this->Internal->drawCheckboxControl(
      painter, rect, numItemsChecked, totalItems, checkstate, this);
    this->Internal->isCheckable[logicalIndex] = true;
    if (checkstate != newCheckstate)
      {
      this->Internal->checkState[logicalIndex] = newCheckstate;
      emit this->checkStateChanged(logicalIndex);
      }
    }
  else
    {
    this->Internal->isCheckable[logicalIndex] = false;
    this->Internal->checkState[logicalIndex] = QVariant(Qt::Unchecked);
    }
}

//----------------------------------------------------------------------------
void pqCheckableHeaderView::mousePressEvent(QMouseEvent *event)
{
  QAbstractItemModel *model = this->model();

  if(model)
    {
    bool active = true;
    if (this->parentWidget())
      {
      active = this->parentWidget()->hasFocus();
      }
    int logicalIndexPressed = logicalIndexAt(event->pos());
    if (this->Internal->isCheckable.contains(logicalIndexPressed) &&
      this->Internal->isCheckable[logicalIndexPressed])
      {
      QStyleOptionButton checkBoxStyleOption;
      QRect checkBoxRect = this->style()->subElementRect(
        QStyle::SE_CheckBoxIndicator,
        &checkBoxStyleOption);
      int buttonMargin = this->style()->pixelMetric(
        QStyle::PM_ButtonMargin, NULL, this);
      int secPos = this->sectionViewportPosition(logicalIndexPressed);
      int secPosX = this->orientation() == Qt::Horizontal ?
        secPos : 0;
      int secPosY = this->orientation() == Qt::Horizontal ?
        0 : secPos;
      if (event->x() <= (secPosX + buttonMargin + checkBoxRect.width()) &&
          event->x() >= (secPosX + buttonMargin) &&
          event->y() <= (secPosY + buttonMargin + checkBoxRect.height()) &&
          event->y() >= (secPosY + buttonMargin))
        {
        if (this->Internal->checkState.contains(logicalIndexPressed))
          {
          if ((this->Internal->checkState[logicalIndexPressed]).toInt() != Qt::Unchecked)
            {
            this->Internal->checkState[logicalIndexPressed] = QVariant(Qt::Unchecked);
            }
          else
            {
            this->Internal->checkState[logicalIndexPressed] = QVariant(Qt::Checked);
            }
          this->Internal->forceCheck = true;
          emit this->checkStateChanged(logicalIndexPressed);
          this->updateModelCheckState(logicalIndexPressed);
          }
        else
          {
          // assuming it was not checked if not registered
          this->Internal->checkState[logicalIndexPressed] =
            QVariant(Qt::Checked);
          }
        this->updateSection(logicalIndexPressed);
        return;
        }
      }
    }
  this->update();
  QHeaderView::mousePressEvent(event);
}

//----------------------------------------------------------------------------
void pqCheckableHeaderView::updateModelCheckState(int section)
{
  // Update the check state of all checkable items in the model based on the
  // checkstate of the header checkbox

  QAbstractItemModel *model = this->model();

  if (!model)
    {
    return;
    }

  // Total number of top-level items in this section
  int totalItems = this->orientation() == Qt::Horizontal ?
    model->rowCount() : model->columnCount();

  bool checkable = false;
  QVariant checked = ((this->Internal->checkState[section]).toInt() ==
    Qt::Unchecked) ? Qt::Unchecked : Qt::Checked;

  if (this->orientation() == Qt::Horizontal)
    {
    for (int i = 0; i < totalItems; i++)
      {
      QModelIndex idx;
      if (this->orientation() == Qt::Horizontal)
        {
        // If headerview is horizontal, the sections are columns and iterate
        // over rows
        idx = model->index(i, section);
        }
      else
        {
        // If headerview is vertical, the sections are rows and iterate over
        // columns
        idx = model->index(section, i);
        }
      Qt::ItemFlags f = model->flags(idx);
      if ((f & Qt::ItemIsUserCheckable) != 0)
        {
        checkable = true;
        model->setData(idx, checked, Qt::CheckStateRole);
        }
      }
    }

}

//----------------------------------------------------------------------------
QVariant pqCheckableHeaderView::getCheckState(int section)
{
  return this->Internal->checkState[section];
}
