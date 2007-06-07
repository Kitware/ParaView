/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelectionHelper.cxx

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

/// \file pqHistogramSelectionHelper.cxx
/// \date 11/8/2006

#include "pqHistogramSelectionHelper.h"

#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartMouseBox.h"
#include "pqHistogramChart.h"
#include "pqHistogramSelection.h"
#include "pqHistogramSelectionModel.h"

#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRect>
#include <QTimer>


class pqHistogramSelectionHelperInternal
{
public:
  pqHistogramSelectionHelperInternal();
  ~pqHistogramSelectionHelperInternal() {}

  /// Stores the previously used list of selected ranges.
  pqHistogramSelectionList Selection;
  int LastBin;    ///< Stores the last bin click.
  int LastValueX; ///< Stores the last value click.
};


//----------------------------------------------------------------------------
pqHistogramSelectionHelperInternal::pqHistogramSelectionHelperInternal()
  : Selection()
{
  this->LastBin = -1;
  this->LastValueX = -1;
}


//----------------------------------------------------------------------------
pqHistogramSelectionHelper::pqHistogramSelectionHelper(
    pqHistogramChart *histogram, QObject *parentObject)
  : pqChartSelectionHelper(parentObject)
{
  this->Internal = new pqHistogramSelectionHelperInternal();
  this->PickStyle = pqHistogramChart::BinRange;
  this->Interact = pqHistogramSelectionHelper::Bin;
  this->SelectMode = this->Interact;
  this->Mode = pqHistogramSelectionHelper::NoMode;
  this->Histogram = histogram;
  this->MoveTimer = 0;
}

pqHistogramSelectionHelper::~pqHistogramSelectionHelper()
{
  delete this->Internal;
}

bool pqHistogramSelectionHelper::handleKeyPress(QKeyEvent *e)
{
  if(e->key() == Qt::Key_M)
    {
    if(this->Interact == pqHistogramSelectionHelper::Value)
      {
      this->setInteractMode(pqHistogramSelectionHelper::ValueMove);
      return true;
      }
    else if(this->Interact == pqHistogramSelectionHelper::ValueMove)
      {
      this->setInteractMode(pqHistogramSelectionHelper::Value);
      return true;
      }
    }

  return false;
}

bool pqHistogramSelectionHelper::handleMousePress(QMouseEvent *e)
{
  if(!this->Histogram || e->button() != Qt::LeftButton)
    {
    return false;
    }

  pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
  if(!model)
    {
    return false;
    }

  // Translate the mouse position to contents coordinates.
  QPoint point = e->pos();
  this->getContentSpace()->translateToContents(point);

  // Make sure the timer is allocated and connected.
  if(!this->MoveTimer)
    {
    this->MoveTimer = new QTimer(this);
    this->MoveTimer->setObjectName("MouseMoveTimeout");
    this->MoveTimer->setSingleShot(true);
    connect(this->MoveTimer, SIGNAL(timeout()), this, SLOT(moveTimeout()));
    }

  // Use a timer to determine if the user will drag the mouse as
  // part of the selection. If the timer expires, finish the
  // interactive selection change.
  pqHistogramSelection range;
  if(this->Interact == pqHistogramSelectionHelper::Bin)
    {
    int bin = this->Histogram->getBinAt(point.x(), point.y(), this->PickStyle);
    range.setType(pqHistogramSelection::Bin);
    range.setRange(bin, bin);
    if(e->modifiers() & Qt::ShiftModifier)
      {
      if(bin != -1)
        {
        model->beginInteractiveChange();
        if(this->Internal->LastBin == -1)
          {
          model->setSelection(range);
          this->Internal->LastBin = bin;
          }
        else
          {
          range.setFirst(this->Internal->LastBin);
          model->setSelection(range);
          }
        }
      }
    else if(e->modifiers() & Qt::ControlModifier)
      {
      if(bin != -1)
        {
        model->beginInteractiveChange();
        model->xorSelection(range);
        this->Internal->LastBin = bin;

        // Set up the selection list so the first click doesn't
        // get changed.
        this->Internal->Selection.clear();
        this->Internal->Selection.append(range);
        }
      else
        {
        this->Internal->Selection.clear();
        }
      }
    else
      {
      model->beginInteractiveChange();
      this->Internal->LastBin = bin;
      if(bin == -1)
        {
        model->selectNone();
        }
      else
        {
        model->setSelection(range);
        }
      }
    }
  else if(this->Interact == pqHistogramSelectionHelper::Value)
    {
      pqChartValue val;
      bool valid = this->Histogram->getValueAt(point.x(), point.y(), val);
      range.setType(pqHistogramSelection::Value);
      range.setRange(val, val);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(valid)
          {
          model->beginInteractiveChange();
          if(this->Internal->LastValueX == -1)
            {
            this->Internal->LastValueX = point.x();
            model->setSelection(range);
            }
          else
            {
            pqChartValue last;
            if(this->Histogram->getValueAt(this->Internal->LastValueX,
                point.y(), last))
              {
              range.setFirst(last);
              model->setSelection(range);
              }
            }
          }
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        if(valid)
          {
          model->beginInteractiveChange();
          this->Internal->LastValueX = point.x();
          model->xorSelection(range);

          // Set up the selection list so the first click doesn't
          // get changed.
          this->Internal->Selection.clear();
          this->Internal->Selection.append(range);
          }
        else
          {
          this->Internal->Selection.clear();
          }
        }
      else
        {
        model->beginInteractiveChange();
        if(valid)
          {
          this->Internal->LastValueX = point.x();
          model->setSelection(range);
          }
        else
          {
          this->Internal->LastValueX = -1;
          model->selectNone();
          }
        }
    }
  else //if(this->Interact == pqHistogramSelectionHelper::ValueMove)
    {
    bool valid = this->Histogram->getValueRangeAt(point.x(), point.y(), range);
    if(valid)
      {
      this->Internal->LastValueX = point.x();
      }
    else
      {
      this->Internal->LastValueX = -1;
      }
    }

  if(model->isInInteractiveChange())
    {
    this->Mode = pqHistogramSelectionHelper::MoveWait;
    this->MoveTimer->start(200);
    emit this->repaintNeeded();
    }

  return true;
}

bool pqHistogramSelectionHelper::handleMouseMove(QMouseEvent *e)
{
  if(!this->Histogram)
    {
    return false;
    }

  pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
  if(!model)
    {
    return false;
    }

  // Translate the mouse position to contents coordinates.
  QPoint point = e->pos();
  this->getContentSpace()->translateToContents(point);

  // Check for the move wait timer. If it is active, cancel
  // the timer so it does not send a selection update.
  if(this->Mode == pqHistogramSelectionHelper::MoveWait)
    {
    this->Mode = pqHistogramSelectionHelper::NoMode;
    if(this->MoveTimer)
      {
      this->MoveTimer->stop();
      }
    }

  if(this->Mode == pqHistogramSelectionHelper::NoMode)
    {
    if(e->buttons() == Qt::LeftButton)
      {
      if(this->Interact == pqHistogramSelectionHelper::Value)
        {
        if(this->Internal->LastValueX != -1)
          {
          model->beginInteractiveChange();
          this->Mode = pqHistogramSelectionHelper::ValueDrag;
          }
        }
      else if(this->Interact == pqHistogramSelectionHelper::ValueMove)
        {
        if(this->Internal->LastValueX != -1)
          {
          model->beginInteractiveChange();
          this->Mode = pqHistogramSelectionHelper::ValueDrag;
          emit this->cursorChangeNeeded(QCursor(Qt::SizeAllCursor));
          }
        }
      else
        {
        model->beginInteractiveChange();
        this->Mode = pqHistogramSelectionHelper::SelectBox;
        }
      }
    }

  if(this->Mode == pqHistogramSelectionHelper::SelectBox)
    {
    // Save a copy of the old area. The union of the old and new
    // areas need to be repainted.
    pqChartMouseBox *mouse = this->getMouseBox();
    QRect area = mouse->Box;
    mouse->adjustBox(point);
    if(area.isValid())
      {
      area = area.unite(mouse->Box);
      }
    else
      {
      area = mouse->Box;
      }

    // Update the selection based on the new selection box.
    if(this->Interact == pqHistogramSelectionHelper::Bin)
      {
      // Block the selection changed signals. A single signal will
      // get sent when the mouse is released.
      pqHistogramSelectionList newSelection;
      this->Histogram->getBinsIn(mouse->Box, newSelection, this->PickStyle);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(!this->Internal->Selection.isEmpty())
          {
          model->subtractSelection(this->Internal->Selection);
          }

        model->addSelection(newSelection);
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        // Use a temporary selection model to find the intersection
        // of the new selection and the previous one.
        pqHistogramSelectionModel temp;
        temp.setSelection(this->Internal->Selection);
        temp.xorSelection(newSelection);
        model->xorSelection(temp.getSelection());
        }
      else
        {
        model->setSelection(newSelection);
        }

      // Adjust the repaint area to include the highlight;
      int extra = this->Histogram->getBinWidth() + 1;
      area.setRight(area.right() + extra);
      area.setLeft(area.left() - extra);
      area.setTop(0);
      area.setBottom(this->getContentSpace()->getContentsHeight());

      // Save the new selection in place of the old one.
      this->Internal->Selection.clear();
      this->Internal->Selection = newSelection;
      }

    // Repaint the affected area.
    this->getContentSpace()->translateFromContents(area);
    emit this->repaintNeeded(area);
    }
  else if(this->Mode == pqHistogramSelectionHelper::ValueDrag)
    {
    if(this->Interact == pqHistogramSelectionHelper::Value)
      {
      QRect area;
      area.setTop(0);
      area.setBottom(this->getContentSpace()->getContentsHeight());
      if(this->Internal->LastValueX < point.x())
        {
        area.setLeft(this->Internal->LastValueX);
        area.setRight(point.x());
        }
      else
        {
        area.setRight(this->Internal->LastValueX);
        area.setLeft(point.x());
        }

      pqHistogramSelectionList newSelection;
      this->Histogram->getValuesIn(area, newSelection);
      if((e->modifiers() & Qt::ControlModifier) &&
          !(e->modifiers() & Qt::ShiftModifier))
        {
        // Use a temporary selection model to find the intersection
        // of the new selection and the previous one.
        pqHistogramSelectionModel temp;
        temp.setSelection(this->Internal->Selection);
        temp.xorSelection(newSelection);
        model->xorSelection(temp.getSelection());
        }
      else
        {
        model->setSelection(newSelection);
        }

      // Adjust the repaint area to include the highlight;
      int extra = this->Histogram->getBinWidth() + 1;
      area.setRight(area.right() + extra);
      area.setLeft(area.left() - extra);

      // Save the new selection in place of the old one.
      this->Internal->Selection.clear();
      this->Internal->Selection = newSelection;

      this->getContentSpace()->translateFromContents(area);
      emit this->repaintNeeded(area);
      }
    else if(this->Internal->LastValueX != point.x())
      {
      // Get the selection range under the previous mouse position
      // and move it to the new position.
      pqHistogramSelection range;
      if(this->Histogram->getValueRangeAt(this->Internal->LastValueX,
          point.y(), range))
        {
        // Get the value offset.
        pqChartAxis *xAxis = this->Histogram->getXAxis();
        const pqChartPixelScale *xScale = xAxis->getPixelValueScale();
        pqChartValue offset = xScale->getValueFor(point.x());
        offset -= xScale->getValueFor(this->Internal->LastValueX);
        if(offset != 0)
          {
          // Save the last position and move the selection.
          model->moveSelection(range, offset);
          if(range.getFirst() == range.getSecond())
            {
            range.moveRange(offset);
            this->Internal->LastValueX = xScale->getPixelFor(range.getFirst());
            }
          else
            {
            this->Internal->LastValueX = point.x();
            }

          emit this->repaintNeeded();
          }
        }
      }
    }

  return this->Mode != pqHistogramSelectionHelper::NoMode;
}

bool pqHistogramSelectionHelper::handleMouseRelease(QMouseEvent *e)
{
  if(!this->Histogram || e->button() != Qt::LeftButton ||
      this->Mode == pqHistogramSelectionHelper::NoMode)
    {
    return false;
    }

  pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
  if(!model)
    {
    return false;
    }

  // Translate the mouse position to contents coordinates.
  QPoint point = e->pos();
  this->getContentSpace()->translateToContents(point);
  if(this->Mode == pqHistogramSelectionHelper::SelectBox)
    {
    this->Mode = pqHistogramSelectionHelper::NoMode;
    model->endInteractiveChange();
    this->Internal->Selection.clear();

    pqChartMouseBox *mouse = this->getMouseBox();
    mouse->adjustBox(point);
    QRect area = mouse->Box;
    mouse->resetBox();
    if(area.isValid())
      {
      this->getContentSpace()->translateFromContents(area);
      emit this->repaintNeeded(area);
      }
    }
  else if(this->Mode == pqHistogramSelectionHelper::ValueDrag)
    {
    this->Mode = pqHistogramSelectionHelper::NoMode;
    model->endInteractiveChange();
    this->Internal->Selection.clear();
    emit this->cursorChangeNeeded(QCursor(Qt::ArrowCursor));
    }
  else //if(this->Mode == pqHistogramSelectionHelper::MoveWait)
    {
    // Cancel the timer and send the selection changed signal.
    this->Mode = pqHistogramSelectionHelper::NoMode;
    if(this->MoveTimer)
      {
      this->MoveTimer->stop();
      }

    model->endInteractiveChange();
    this->Internal->Selection.clear();
    }

  return true;
}

bool pqHistogramSelectionHelper::handleMouseDoubleClick(QMouseEvent *)
{
  return false;
}

void pqHistogramSelectionHelper::setInteractMode(
    pqHistogramSelectionHelper::InteractMode mode)
{
  if(mode != this->Interact)
    {
    if(this->Interact == pqHistogramSelectionHelper::ValueMove)
      {
      this->Internal->LastValueX = -1;
      }

    // If the user is switching between bin and value selection,
    // clear the selection.
    bool clearNeeded = this->Interact == pqHistogramSelectionHelper::Bin ||
        mode == pqHistogramSelectionHelper::Bin;
    this->Interact = mode;
    if(clearNeeded && this->Histogram)
      {
      pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
      if(model)
        {
        model->selectNone();
        }
      }
    }
}

void pqHistogramSelectionHelper::moveTimeout()
{
  if(this->Histogram)
    {
    pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
    if(model)
      {
      model->selectNone();
      }
    }
}


