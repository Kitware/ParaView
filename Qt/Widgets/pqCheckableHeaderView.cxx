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
    this->Style = nullptr;
    this->forceCheck = false;
  }
  ~pqCheckableHeaderViewInternal() = default;

  /// \brief
  ///   Returns the placeholder rect for a checkbox in the section header.
  QRect checkBoxRect(const QRect& sourceRect, const QAbstractItemView* view) const;

  /// \brief
  ///   Draw the checkbox control
  ///   Based on two scenarios:
  ///   1. If any of the item checkboxes are changed, the section header
  ///   checkbox needs to be updated.
  ///   2. If the section header checkbox is clicked and its state needs
  ///   to be toggled.
  ///   It returns the new checkbox state
  ///   \arg checkState: State to force the header checkbox to. Used when
  ///   clicked on the checkbox.
  QVariant drawCheckboxControl(QPainter* painter, const QRect& rect, QRect& chbRect,
    int numItemsChecked, int totalItems, QVariant checkState, const QAbstractItemView* view);

  QStyle* Style;

  /// \brief
  ///   Map of (section,checkable) values indicating whether
  ///   the section is checkable
  QHash<int, bool> isCheckable;

  /// \brief
  ///   Map of (section,checkstate) values indicating whether
  ///   the section checkbox is checked/partially checked/unchecked
  QHash<int, QVariant> checkState;

  /// \brief
  ///   Map of (section,checkBoxRect) values indicating the top-left corner
  ///   position and size of the section checkbox
  QHash<int, QRect> checkBoxRectHash;

  /// \brief
  ///   Boolean to force check/uncheck state of header checkbox
  bool forceCheck;
};

//----------------------------------------------------------------------------
QRect pqCheckableHeaderViewInternal::checkBoxRect(
  const QRect& sourceRect, const QAbstractItemView* view) const
{
  QStyleOptionButton checkBoxStyleOption;
  QRect cboxRect = this->Style->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxStyleOption);
  int buttonMargin = this->Style->pixelMetric(QStyle::PM_ButtonMargin, nullptr, view);

  int ch = cboxRect.height();
  int cw = cboxRect.width();
  int sh = sourceRect.height();
  int sw = sourceRect.width();
  int bh = buttonMargin;
  int bw = buttonMargin;

  // Logic to make sure the checkbox is contained in the viewable area of the
  // section header rect.
  if ((ch + 2 * bh) > sh)
  {
    if (ch > sh)
    {
      bh = 0;
      cboxRect.setHeight(sh);
    }
    else if (ch < sh)
    {
      bh = static_cast<int>((sh - ch) / 2.0);
    }
    else
    {
      // ch == sh
      bh = 0;
    }
  }

  if ((cw + 2 * bw) > sw)
  {
    if (cw > sw)
    {
      bw = 0;
      cboxRect.setWidth(sw);
    }
    else if (cw < sw)
    {
      bw = static_cast<int>((sw - cw) / 2.0);
    }
    else
    {
      // cw == sw
      bw = 0;
    }
  }

  QSize chbSize = QSize(cw, ch);
  chbSize.scale(cboxRect.size(), Qt::KeepAspectRatio);

  QPoint checkBoxPoint(sourceRect.x() + bw, sourceRect.y() + bh);
  return QRect(checkBoxPoint, chbSize);
}

//----------------------------------------------------------------------------
QVariant pqCheckableHeaderViewInternal::drawCheckboxControl(QPainter* painter, const QRect& rect,
  QRect& chbRect, int numItemsChecked, int totalItems, QVariant curCheckState,
  const QAbstractItemView* view)
{
  QVariant isChecked;

  QStyleOptionButton option;
  option.rect = this->checkBoxRect(rect, view);
  chbRect = option.rect;
  option.state |= QStyle::State_Enabled;

  if (this->forceCheck)
  {
    if (curCheckState.toInt() == Qt::Unchecked)
    {
      option.state |= QStyle::State_Off;
    }
    else
    {
      option.state |= QStyle::State_On;
    }
    isChecked = curCheckState;
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
  this->Style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);

  // Reset the forceCheck boolean once the checkbox is drawn
  this->forceCheck = false;

  return isChecked;
}

//----------------------------------------------------------------------------
pqCheckableHeaderView::pqCheckableHeaderView(Qt::Orientation orientation, QWidget* parentObject)
  : QHeaderView(orientation, parentObject)
{
  this->Internal = new pqCheckableHeaderViewInternal();
  this->Internal->Style = this->style();
  setSectionsClickable(true);
  QObject::connect(this, SIGNAL(checkStateChanged(int)), this, SLOT(updateSection(int)));
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
void pqCheckableHeaderView::paintSection(
  QPainter* painter, const QRect& sectionRect, int logicalIdx) const
{
  painter->save();
  QHeaderView::paintSection(painter, sectionRect, logicalIdx);
  painter->restore();

  QAbstractItemModel* theModel = this->model();
  if (!theModel)
  {
    return;
  }

  // Total number of top-level items in this section
  int totalItems =
    this->orientation() == Qt::Horizontal ? theModel->rowCount() : theModel->columnCount();

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
        idx = theModel->index(i, logicalIdx);
      }
      else
      {
        // If headerview is vertical, the sections are rows and iterate over
        // columns
        idx = theModel->index(logicalIdx, i);
      }
      Qt::ItemFlags f = theModel->flags(idx);
      if ((f & Qt::ItemIsUserCheckable) != 0)
      {
        checkable = true;
        QVariant value = theModel->data(idx, Qt::CheckStateRole);
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
    if (this->Internal->checkState.contains(logicalIdx))
    {
      checkstate = this->Internal->checkState[logicalIdx];
    }
    QRect chbRect;
    QVariant newCheckstate = this->Internal->drawCheckboxControl(
      painter, sectionRect, chbRect, numItemsChecked, totalItems, checkstate, this);
    this->Internal->isCheckable[logicalIdx] = true;
    this->Internal->checkBoxRectHash[logicalIdx] = chbRect;
    if (checkstate != newCheckstate)
    {
      this->Internal->checkState[logicalIdx] = newCheckstate;
      Q_EMIT this->checkStateChanged(logicalIdx);
    }
  }
  else
  {
    this->Internal->isCheckable[logicalIdx] = false;
    this->Internal->checkState[logicalIdx] = QVariant(Qt::Unchecked);
  }
}

//----------------------------------------------------------------------------
void pqCheckableHeaderView::mousePressEvent(QMouseEvent* evt)
{
  QAbstractItemModel* theModel = this->model();

  if (theModel)
  {
    // bool active = true;
    // if (this->parentWidget())
    //  {
    //  active = this->parentWidget()->hasFocus();
    //  }
    int logicalIndexPressed = logicalIndexAt(evt->pos());
    if (this->Internal->isCheckable.contains(logicalIndexPressed) &&
      this->Internal->isCheckable[logicalIndexPressed])
    {
      int secPos = this->sectionViewportPosition(logicalIndexPressed);
      int secPosX = this->orientation() == Qt::Horizontal ? secPos : 0;
      int secPosY = this->orientation() == Qt::Horizontal ? 0 : secPos;
      QRect chbRect = this->Internal->checkBoxRectHash[logicalIndexPressed];
      if (evt->x() <= (secPosX + chbRect.right()) && evt->x() >= (secPosX + chbRect.left()) &&
        evt->y() <= (secPosY + chbRect.bottom()) && evt->y() >= (secPosY + chbRect.top()))
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
          Q_EMIT this->checkStateChanged(logicalIndexPressed);
          this->updateModelCheckState(logicalIndexPressed);
        }
        else
        {
          // assuming it was not checked if not registered
          this->Internal->checkState[logicalIndexPressed] = QVariant(Qt::Checked);
        }
        this->updateSection(logicalIndexPressed);
        return;
      }
    }
  }
  this->update();
  QHeaderView::mousePressEvent(evt);
}

//----------------------------------------------------------------------------
void pqCheckableHeaderView::updateModelCheckState(int section)
{
  // Update the check state of all checkable items in the model based on the
  // checkstate of the header checkbox

  QAbstractItemModel* theModel = this->model();

  if (!theModel)
  {
    return;
  }

  // Total number of top-level items in this section
  int totalItems =
    this->orientation() == Qt::Horizontal ? theModel->rowCount() : theModel->columnCount();

  // bool checkable = false;
  QVariant checked =
    ((this->Internal->checkState[section]).toInt() == Qt::Unchecked) ? Qt::Unchecked : Qt::Checked;

  if (this->orientation() == Qt::Horizontal)
  {
    for (int i = 0; i < totalItems; i++)
    {
      QModelIndex idx;
      if (this->orientation() == Qt::Horizontal)
      {
        // If headerview is horizontal, the sections are columns and iterate
        // over rows
        idx = theModel->index(i, section);
      }
      else
      {
        // If headerview is vertical, the sections are rows and iterate over
        // columns
        idx = theModel->index(section, i);
      }
      Qt::ItemFlags f = theModel->flags(idx);
      if ((f & Qt::ItemIsUserCheckable) != 0)
      {
        // checkable = true;
        theModel->setData(idx, checked, Qt::CheckStateRole);
      }
    }
  }
}

//----------------------------------------------------------------------------
QVariant pqCheckableHeaderView::getCheckState(int section)
{
  return this->Internal->checkState[section];
}
