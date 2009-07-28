/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseSelection.cxx

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

/// \file pqChartMouseSelection.cxx
/// \date 6/20/2007

#include "pqChartMouseSelection.h"

#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartMouseBox.h"
#include "pqChartPixelScale.h"
#include "pqHistogramSelectionModel.h"

#include <QCursor>
#include <QMap>
#include <QMouseEvent>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QTimer>


class pqChartMouseSelectionInternal
{
public:
  enum SelectionMode
    {
    NoMode = -1,
    HistogramBin = 0,
    HistogramValue,
    HistogramMoveRange
    };

  enum SelectionState
    {
    NoState = 0,
    SelectBox,
    SelectDrag,
    DragMove
    };

public:
  pqChartMouseSelectionInternal();
  ~pqChartMouseSelectionInternal() {}

  QStringList Modes; ///< Stores the list of mode names.
  QString Current;   ///< Stores the current mode name.
};


class pqChartMouseSelectionHistogram
{
public:
  pqChartMouseSelectionHistogram();
  ~pqChartMouseSelectionHistogram() {}

  /// Stores the previously used list of selected ranges.
  pqHistogramSelectionList Selection;

  /// Stores the bin pick mode for histogram selection.
  pqHistogramChart::BinPickMode PickStyle;

  QPair<int, int> Modes;   ///< Stores the mode range.
  pqHistogramChart *Chart; ///< Stores the histogram chart.
  int LastBin;             ///< Stores the last bin click.
  int LastValueX;          ///< Stores the last value click.
  bool DelaySelection;     ///< True if a selection signal is needed.
};


//----------------------------------------------------------------------------
pqChartMouseSelectionInternal::pqChartMouseSelectionInternal()
  : Modes(), Current()
{
  // Add the names for all the modes. The names should be in the same
  // order as the enum.
  this->Modes << "Histogram-Bin"
      << "Histogram-Value"
      << "Histogram-MoveRange";
}


//----------------------------------------------------------------------------
pqChartMouseSelectionHistogram::pqChartMouseSelectionHistogram()
  : Selection(), Modes(-1, -1)
{
  this->PickStyle = pqHistogramChart::BinRange;
  this->Chart = 0;
  this->LastBin = -1;
  this->LastValueX = -1;
  this->DelaySelection = false;

  // Set up the enum range objects.
  this->Modes.first = pqChartMouseSelectionInternal::HistogramBin;
  this->Modes.second = pqChartMouseSelectionInternal::HistogramMoveRange;
}


//----------------------------------------------------------------------------
pqChartMouseSelection::pqChartMouseSelection(QObject *parentObject)
  : pqChartMouseFunction(parentObject)
{
  this->Internal = new pqChartMouseSelectionInternal();
  this->Histogram = new pqChartMouseSelectionHistogram();
  this->MouseBox = 0;
  this->Mode = pqChartMouseSelectionInternal::NoMode;
  this->State = pqChartMouseSelectionInternal::NoState;
}

pqChartMouseSelection::~pqChartMouseSelection()
{
  delete this->Internal;
  delete this->Histogram;
}

pqHistogramChart *pqChartMouseSelection::getHistogram() const
{
  return this->Histogram->Chart;
}

void pqChartMouseSelection::setHistogram(pqHistogramChart *histogram)
{
  if(this->Histogram->Chart != histogram)
    {
    // Notify observers that the mode availabilities have changed.
    this->Histogram->Chart = histogram;
    emit this->modeAvailabilityChanged();
    }
}

const QString &pqChartMouseSelection::getSelectionMode() const
{
  return this->Internal->Current;
}

void pqChartMouseSelection::getAllModes(QStringList &list) const
{
  list << this->Internal->Modes;
}

void pqChartMouseSelection::getAvailableModes(QStringList &list) const
{
  if(this->Histogram->Chart)
    {
    int i = this->Histogram->Modes.first;
    for( ; i <= this->Histogram->Modes.second; i++)
      {
      list << this->Internal->Modes[i];
      }
    }
}

pqHistogramChart::BinPickMode pqChartMouseSelection::getPickStyle() const
{
  return this->Histogram->PickStyle;
}

void pqChartMouseSelection::setPickStyle(pqHistogramChart::BinPickMode style)
{
  this->Histogram->PickStyle = style;
}

bool pqChartMouseSelection::mousePressEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  bool handled = false;
  if(this->Mode >= this->Histogram->Modes.first &&
      this->Mode <= this->Histogram->Modes.second)
    {
    pqHistogramSelectionModel *model = this->Histogram->Chart ?
        this->Histogram->Chart->getSelectionModel() : 0;
    if(model)
      {
      // Translate the mouse position to contents coordinates.
      QPoint point = e->pos();
      contents->translateToContents(point);

      if(this->Mode == pqChartMouseSelectionInternal::HistogramBin)
        {
        this->mousePressHistogramBin(model, point, e->modifiers());
        }
      else if(this->Mode == pqChartMouseSelectionInternal::HistogramValue)
        {
        this->mousePressHistogramValue(model, point, e->modifiers());
        }
      else // pqChartMouseSelectionInternal::HistogramMoveRange
        {
        this->mousePressHistogramMove(point);
        }

      handled = true;
      if(model->isInInteractiveChange())
        {
        // If a selection change is made, delay the model change
        // signal until mouse release.
        this->Histogram->DelaySelection = true;
        }
      }
    }

  return handled;
}

bool pqChartMouseSelection::mouseMoveEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  if(!this->isMouseOwner())
    {
    if(this->Mode >= this->Histogram->Modes.first &&
        this->Mode <= this->Histogram->Modes.second)
      {
      // Make sure the required information is available before trying
      // to enter a mouse state.
      pqHistogramSelectionModel *model = this->Histogram->Chart ?
          this->Histogram->Chart->getSelectionModel() : 0;
      bool isStateAvailable = true;
      if(this->Mode == pqChartMouseSelectionInternal::HistogramBin)
        {
        isStateAvailable = this->MouseBox != 0;
        }
      else
        {
        isStateAvailable = this->Histogram->LastValueX != -1;
        }

      if(model && isStateAvailable)
        {
        emit this->interactionStarted(this);
        if(this->isMouseOwner())
          {
          this->Histogram->DelaySelection = false;
          model->beginInteractiveChange();
          if(this->Mode == pqChartMouseSelectionInternal::HistogramBin)
            {
            this->State = pqChartMouseSelectionInternal::SelectBox;
            }
          else if(this->Mode == pqChartMouseSelectionInternal::HistogramValue)
            {
            this->State = pqChartMouseSelectionInternal::SelectDrag;
            }
          else // pqChartMouseSelectionInternal::HistogramMoveRange
            {
            this->State = pqChartMouseSelectionInternal::DragMove;
            emit this->cursorChangeRequested(QCursor(Qt::SizeAllCursor));
            }
          }
        }
      }
    }

  // Translate the mouse position to contents coordinates.
  QPoint point = e->pos();
  contents->translateToContents(point);

  // Note: The state should only be set if this is the mouse owner.
  if(this->State == pqChartMouseSelectionInternal::SelectBox)
    {
    this->mouseMoveSelectBox(contents, point, e->modifiers());
    }
  else if(this->State == pqChartMouseSelectionInternal::SelectDrag)
    {
    this->mouseMoveSelectDrag(contents, point, e->modifiers());
    }
  else if(this->State == pqChartMouseSelectionInternal::DragMove)
    {
    this->mouseMoveDragMove(point);
    }

  return this->isMouseOwner();
}

bool pqChartMouseSelection::mouseReleaseEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  if(this->Histogram->DelaySelection)
    {
    this->Histogram->Chart->getSelectionModel()->endInteractiveChange();
    }

  if(!this->isMouseOwner())
    {
    return false;
    }

  if(this->State == pqChartMouseSelectionInternal::SelectBox ||
      this->State == pqChartMouseSelectionInternal::SelectDrag ||
      this->State == pqChartMouseSelectionInternal::DragMove)
    {
    this->Histogram->Chart->getSelectionModel()->endInteractiveChange();
    this->Histogram->Selection.clear();
    if(this->State == pqChartMouseSelectionInternal::SelectBox)
      {
      // Repaint the mouse box area.
      QRect area;
      QPoint point = e->pos();
      contents->translateToContents(point);
      this->MouseBox->getRectangle(area);
      this->MouseBox->adjustRectangle(point);
      this->MouseBox->getUnion(area);
      this->MouseBox->resetRectangle();
      if(area.isValid())
        {
        contents->translateFromContents(area);
        emit this->repaintNeeded(area);
        }
      }
    else if(this->State == pqChartMouseSelectionInternal::DragMove)
      {
      emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
      }

    this->State = pqChartMouseSelectionInternal::NoState;
    emit this->interactionFinished(this);
    }

  return true;
}

bool pqChartMouseSelection::mouseDoubleClickEvent(QMouseEvent *,
    pqChartContentsSpace *)
{
  return false;
}

void pqChartMouseSelection::setSelectionMode(const QString &mode)
{
  int index = this->Internal->Modes.indexOf(mode);
  if(index != this->Mode)
    {
    if(this->Mode == pqChartMouseSelectionInternal::HistogramMoveRange)
      {
      this->Histogram->LastValueX = -1;
      }

    // When switching selection modes, clear the current selection.
    // An exception to this rule is when switching between value and
    // move range modes.
    bool clearNeeded =
        !((index == pqChartMouseSelectionInternal::HistogramValue &&
        this->Mode == pqChartMouseSelectionInternal::HistogramMoveRange) ||
        (this->Mode == pqChartMouseSelectionInternal::HistogramValue &&
        index == pqChartMouseSelectionInternal::HistogramMoveRange));
    if(clearNeeded && this->Histogram->Chart)
      {
      pqHistogramSelectionModel *model =
          this->Histogram->Chart->getSelectionModel();
      if(model)
        {
        model->selectNone();
        }
      }

    // If the index is not found, it will be -1 (NoMode).
    this->Mode = index;
    if(this->Mode == pqChartMouseSelectionInternal::NoMode)
      {
      this->Internal->Current.clear();
      }
    else
      {
      this->Internal->Current = mode;
      }

    // Notify observers that the mode has changed.
    emit this->selectionModeChanged(this->Internal->Current);
    }
}

void pqChartMouseSelection::mousePressHistogramBin(
      pqHistogramSelectionModel *model, const QPoint &point,
      Qt::KeyboardModifiers modifiers)
{
  pqHistogramSelection range;
  int bin = this->Histogram->Chart->getBinAt(point.x(), point.y(),
      this->Histogram->PickStyle);
  range.setType(pqHistogramSelection::Bin);
  range.setRange(bin, bin);
  if(modifiers & Qt::ShiftModifier)
    {
    if(bin != -1)
      {
      model->beginInteractiveChange();
      if(this->Histogram->LastBin == -1)
        {
        model->setSelection(range);
        this->Histogram->LastBin = bin;
        }
      else
        {
        range.setFirst(this->Histogram->LastBin);
        model->setSelection(range);
        }
      }
    }
  else if(modifiers & Qt::ControlModifier)
    {
    if(bin != -1)
      {
      model->beginInteractiveChange();
      model->xorSelection(range);
      this->Histogram->LastBin = bin;

      // Set up the selection list so the first click doesn't get
      // changed.
      this->Histogram->Selection.clear();
      this->Histogram->Selection.append(range);
      }
    else
      {
      this->Histogram->Selection.clear();
      }
    }
  else
    {
    model->beginInteractiveChange();
    this->Histogram->LastBin = bin;
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

void pqChartMouseSelection::mousePressHistogramValue(
      pqHistogramSelectionModel *model, const QPoint &point,
      Qt::KeyboardModifiers modifiers)
{
  pqChartValue value;
  pqHistogramSelection range;
  bool valid = this->Histogram->Chart->getValueAt(point.x(), point.y(), value);
  range.setType(pqHistogramSelection::Value);
  range.setRange(value, value);
  if(modifiers & Qt::ShiftModifier)
    {
    if(valid)
      {
      model->beginInteractiveChange();
      if(this->Histogram->LastValueX == -1)
        {
        this->Histogram->LastValueX = point.x();
        model->setSelection(range);
        }
      else if(this->Histogram->Chart->getValueAt(
          this->Histogram->LastValueX, point.y(), value))
        {
        range.setFirst(value);
        model->setSelection(range);
        }
      }
    }
  else if(modifiers & Qt::ControlModifier)
    {
    if(valid)
      {
      model->beginInteractiveChange();
      this->Histogram->LastValueX = point.x();
      model->xorSelection(range);

      // Set up the selection list so the first click doesn't get
      // changed.
      this->Histogram->Selection.clear();
      this->Histogram->Selection.append(range);
      }
    else
      {
      this->Histogram->Selection.clear();
      }
    }
  else
    {
    model->beginInteractiveChange();
    if(valid)
      {
      this->Histogram->LastValueX = point.x();
      model->setSelection(range);
      }
    else
      {
      this->Histogram->LastValueX = -1;
      model->selectNone();
      }
    }
}

void pqChartMouseSelection::mousePressHistogramMove(const QPoint &point)
{
  pqHistogramSelection range;
  bool valid = this->Histogram->Chart->getValueRangeAt(
      point.x(), point.y(), range);
  if(valid)
    {
    this->Histogram->LastValueX = point.x();
    }
  else
    {
    this->Histogram->LastValueX = -1;
    }
}

void pqChartMouseSelection::mouseMoveSelectBox(pqChartContentsSpace *contents,
    const QPoint &point, Qt::KeyboardModifiers modifiers)
{
  // Save a copy of the old area. The union of the old and new
  // areas need to be repainted.
  QRect area;
  this->MouseBox->getRectangle(area);
  this->MouseBox->adjustRectangle(point);
  this->MouseBox->getUnion(area);

  // Update the selection based on the new selection box.
  // Block the selection changed signals. A single signal will
  // get sent when the mouse is released.
  QRect box;
  pqHistogramSelectionList newSelection;
  this->MouseBox->getRectangle(box);
  this->Histogram->Chart->getBinsIn(box, newSelection,
      this->Histogram->PickStyle);
  pqHistogramSelectionModel *model =
      this->Histogram->Chart->getSelectionModel();
  if(modifiers & Qt::ShiftModifier)
    {
    if(!this->Histogram->Selection.isEmpty())
      {
      model->subtractSelection(this->Histogram->Selection);
      }

    model->addSelection(newSelection);
    }
  else if(modifiers & Qt::ControlModifier)
    {
    // Use a temporary selection model to find the intersection
    // of the new selection and the previous one.
    pqHistogramSelectionModel temp;
    temp.setSelection(this->Histogram->Selection);
    temp.xorSelection(newSelection);
    model->xorSelection(temp.getSelection());
    }
  else
    {
    model->setSelection(newSelection);
    }

  // Save the new selection in place of the old one.
  this->Histogram->Selection.clear();
  this->Histogram->Selection = newSelection;

  // Repaint the affected area.
  contents->translateFromContents(area);
  emit this->repaintNeeded(area);
}

void pqChartMouseSelection::mouseMoveSelectDrag(pqChartContentsSpace *contents,
    const QPoint &point, Qt::KeyboardModifiers modifiers)
{
  QRect area;
  area.setTop(0);
  area.setBottom(contents->getContentsHeight());
  if(this->Histogram->LastValueX < point.x())
    {
    area.setLeft(this->Histogram->LastValueX);
    area.setRight(point.x());
    }
  else
    {
    area.setRight(this->Histogram->LastValueX);
    area.setLeft(point.x());
    }

  pqHistogramSelectionList newSelection;
  this->Histogram->Chart->getValuesIn(area, newSelection);
  if((modifiers & Qt::ControlModifier) && !(modifiers & Qt::ShiftModifier))
    {
    // Use a temporary selection model to find the intersection
    // of the new selection and the previous one.
    pqHistogramSelectionModel temp;
    temp.setSelection(this->Histogram->Selection);
    temp.xorSelection(newSelection);
    this->Histogram->Chart->getSelectionModel()->xorSelection(
        temp.getSelection());
    }
  else
    {
    this->Histogram->Chart->getSelectionModel()->setSelection(newSelection);
    }

  // Save the new selection in place of the old one.
  this->Histogram->Selection.clear();
  this->Histogram->Selection = newSelection;
}

void pqChartMouseSelection::mouseMoveDragMove(const QPoint &point)
{
  if(this->Histogram->LastValueX != point.x())
    {
    // Get the selection range under the previous mouse position
    // and move it to the new position.
    pqHistogramSelection range;
    if(this->Histogram->Chart->getValueRangeAt(this->Histogram->LastValueX,
        point.y(), range))
      {
      // Get the value offset.
      pqChartValue offset, offset2;
      pqChartAxis *xAxis = this->Histogram->Chart->getXAxis();
      const pqChartPixelScale *xScale = xAxis->getPixelValueScale();
      xScale->getValue(point.x(), offset);
      xScale->getValue(this->Histogram->LastValueX, offset2);
      offset -= offset2;
      if(offset != 0)
        {
        // Save the last position and move the selection.
        this->Histogram->Chart->getSelectionModel()->moveSelection(
            range, offset);
        if(range.getFirst() == range.getSecond())
          {
          range.moveRange(offset);
          this->Histogram->LastValueX = xScale->getPixel(range.getFirst());
          }
        else
          {
          this->Histogram->LastValueX = point.x();
          }
        }
      }
    }
}


