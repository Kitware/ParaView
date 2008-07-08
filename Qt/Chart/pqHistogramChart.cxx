/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChart.cxx

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

/// \file pqHistogramChart.cxx
/// \date 2/14/2007

#include "pqHistogramChart.h"

#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartAxisModel.h"
#include "pqChartContentsSpace.h"
#include "pqHistogramChartOptions.h"
#include "pqHistogramModel.h"
#include "pqChartPixelScale.h"

#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QtDebug>
#include <QVector>


class pqHistogramChartInternal
{
public:
  pqHistogramChartInternal();
  ~pqHistogramChartInternal() {}

  QVector<QRectF> Items;      ///< The list of histogram bars.
  QVector<QRectF> Highlights; ///< The list of highlighted ranges.
  QRect Bounds;              ///< Stores the chart bounds.
  QRect Contents;            ///< Stores the contents area.
};


//----------------------------------------------------------------------------
pqHistogramChartInternal::pqHistogramChartInternal()
  : Items(), Highlights(), Bounds(), Contents()
{
}


//----------------------------------------------------------------------------
pqHistogramChart::pqHistogramChart(QObject *parentObject)
  : pqChartLayer(parentObject)
{
  this->Internal = new pqHistogramChartInternal();
  this->Options = new pqHistogramChartOptions(this);
  this->Axes = pqHistogramChart::BottomLeft;
  this->Model = 0;
  this->Selection = new pqHistogramSelectionModel(this);
  this->InModelChange = false;

  // Listen for option changes.
  this->connect(this->Options, SIGNAL(optionsChanged()),
      this, SIGNAL(repaintNeeded()));

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const pqHistogramSelectionList &)),
      this, SLOT(updateHighlights()));
}

pqHistogramChart::~pqHistogramChart()
{
  delete this->Internal;
  delete this->Options;
  delete this->Selection;
}

void pqHistogramChart::setChartAxes(ChartAxes axes)
{
  if(this->Axes != axes)
    {
    this->Axes = axes;
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

pqChartAxis *pqHistogramChart::getXAxis() const
{
  pqChartArea *chartArea = this->getChartArea();
  if(!chartArea)
    {
    return 0;
    }

  if(this->Axes == pqHistogramChart::BottomLeft ||
      this->Axes == pqHistogramChart::BottomRight)
    {
    return chartArea->getAxis(pqChartAxis::Bottom);
    }
  else
    {
    return chartArea->getAxis(pqChartAxis::Top);
    }
}

pqChartAxis *pqHistogramChart::getYAxis() const
{
  pqChartArea *chartArea = this->getChartArea();
  if(!chartArea)
    {
    return 0;
    }

  if(this->Axes == pqHistogramChart::BottomLeft ||
      this->Axes == pqHistogramChart::TopLeft)
    {
    return chartArea->getAxis(pqChartAxis::Left);
    }
  else
    {
    return chartArea->getAxis(pqChartAxis::Right);
    }
  return 0;
}

void pqHistogramChart::setModel(pqHistogramModel *model)
{
  if(this->Model == model)
    {
    return;
    }

  // Clean up the current chart data and highlights. Make sure the
  // selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();
  this->Internal->Items.clear();
  this->Internal->Highlights.clear();

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  this->Selection->setModel(model);
  if(this->Model)
    {
    // Connect to the new model's signals.
    this->connect(this->Model, SIGNAL(histogramReset()),
        this, SLOT(handleModelReset()));
    this->connect(this->Model, SIGNAL(aboutToInsertBins(int, int)),
        this, SLOT(startBinInsertion(int, int)));
    this->connect(this->Model, SIGNAL(binsInserted()),
        this, SLOT(finishBinInsertion()));
    this->connect(this->Model, SIGNAL(aboutToRemoveBins(int, int)),
        this, SLOT(startBinRemoval(int, int)));
    this->connect(this->Model, SIGNAL(binsRemoved()),
        this, SLOT(finishBinRemoval()));
    this->connect(this->Model, SIGNAL(histogramRangeChanged()),
        this, SIGNAL(rangeChanged()));
    }

  // Set up the axis ranges and update the chart layout.
  emit this->rangeChanged();
  emit this->layoutNeeded();

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

int pqHistogramChart::getBinAt(int x, int y,
    pqHistogramChart::BinPickMode mode) const
{
  if(this->Internal->Contents.isValid() &&
      this->Internal->Contents.contains(x, y))
    {
    QVector<QRectF>::Iterator iter = this->Internal->Items.begin();
    for(int i = 0; iter != this->Internal->Items.end(); iter++, i++)
      {
      if(mode == pqHistogramChart::BinRange && iter->isValid() &&
          iter->left() < x && x < iter->right())
        {
        return i;
        }
      else if(iter->isValid() && iter->contains(x, y))
        {
        return i;
        }
      }
    }

  return -1;
}

bool pqHistogramChart::getValueAt(int x, int y, pqChartValue &value) const
{
  const pqChartPixelScale *scale = this->getXAxis()->getPixelValueScale();
  if(this->Internal->Contents.isValid() && scale->isValid() &&
      this->Internal->Contents.contains(x, y))
    {
    pqChartValue range;
    scale->getValueRange(range);
    if(range.getType() == pqChartValue::IntValue)
      {
      // Adjust the pick location if the pixel to value ratio is
      // bigger than one. This will help find the closest value.
      int pixel = 0;
      if(range != 0)
        {
        pixel = scale->getPixelRange() / range;
        }

      if(pixel < 0)
        {
        pixel = -pixel;
        }

      if(pixel > 1)
        {
        x += (pixel/2) + 1;
        }
      }

    scale->getValue(x, value);
    return true;
    }

  return false;
}

bool pqHistogramChart::getValueRangeAt(int x, int y,
    pqHistogramSelection &range) const
{
  const pqChartPixelScale *scale = this->getXAxis()->getPixelValueScale();
  if(this->Internal->Contents.isValid() && scale->isValid() &&
    this->Internal->Contents.contains(x, y))
    {
    // Make sure the selection type is 'Value'.
    if(this->Selection->getType() == pqHistogramSelection::Value)
      {
      pqChartValue diff;
      const pqHistogramSelectionList &list = this->Selection->getSelection();
      scale->getValueRange(diff);
      if(diff.getType() == pqChartValue::IntValue)
        {
        // Adjust the pick location if the pixel to value ratio is
        // bigger than one. This will help find the closest value.
        int pixel = 0;
        if(diff != 0)
          {
          pixel = scale->getPixelRange() / diff;
          }
        if(pixel < 0)
          {
          pixel = -pixel;
          }
        if(pixel > 1)
          {
          x += (pixel / 2) + 1;
          }
        }

      // Search through the current selection list.
      pqChartValue value;
      scale->getValue(x, value);
      pqHistogramSelectionList::ConstIterator iter = list.begin();
      for( ; iter != list.end(); ++iter)
        {
        if(iter->getFirst() <= value)
          {
          if(iter->getSecond() >= value)
            {
            range.setType(iter->getType());
            range.setRange(iter->getFirst(), iter->getSecond());
            return true;
            }
          }
        else
          {
          break;
          }
        }
      }
    }

  return false;
}

void pqHistogramChart::getBinsIn(const QRect &area,
    pqHistogramSelectionList &list, pqHistogramChart::BinPickMode mode) const
{
  if(this->Internal->Contents.isValid() && area.isValid() &&
      area.intersects(this->Internal->Contents))
    {
    pqChartValue i((int)0);
    QVector<QRectF>::Iterator iter = this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      if(area.right() < iter->left())
        {
        break;
        }

      bool intersection = false;
      if(mode == pqHistogramChart::BinRange && iter->isValid() &&
          iter->left() < area.right() && iter->right() > area.left())
        {
        intersection = true;
        }
      else if(iter->isValid() && iter->intersects(area))
        {
        intersection = true;
        }

      if(intersection)
        {
        pqHistogramSelection selection(i, i);
        selection.setType(pqHistogramSelection::Bin);
        list.append(selection);
        }
      }

    if(list.size() > 0)
      {
      pqHistogramSelectionModel::sortAndMerge(list);
      }
    }
}

void pqHistogramChart::getValuesIn(const QRect &area,
    pqHistogramSelectionList &list) const
{
  if(!area.isValid() || !this->Internal->Contents.isValid())
    {
    return;
    }

  const pqChartPixelScale *scale = this->getXAxis()->getPixelValueScale();
  if(!scale->isValid() || !area.intersects(this->Internal->Contents))
    {
    return;
    }

  pqChartValue left, right;
  QRect i = area.intersect(this->Internal->Contents);
  if(this->getValueAt(i.left(), i.top(), left) &&
      this->getValueAt(i.right(), i.top(), right))
    {
    pqHistogramSelection selection(left, right);
    selection.setType(pqHistogramSelection::Value);
    list.append(selection);
    }
}

void pqHistogramChart::getSelectionArea(const pqHistogramSelectionList &list,
    QRect &area) const
{
  if(list.isEmpty())
    {
    return;
    }

  const pqHistogramSelection &first = list.first();
  const pqHistogramSelection &last = list.last();
  if(first.getType() != last.getType() ||
      first.getType() == pqHistogramSelection::None)
    {
    qDebug() << "Invalid histogram selection list.";
    return;
    }

  if(first.getType() == pqHistogramSelection::Bin)
    {
    int leftBin = first.getFirst().getIntValue();
    int rightBin = last.getSecond().getIntValue();
    if(rightBin < leftBin)
      {
      leftBin = rightBin;
      rightBin = last.getFirst().getIntValue();
      }

    if(leftBin >= 0 && leftBin < this->Internal->Items.size() &&
        rightBin >= 0 && rightBin < this->Internal->Items.size())
      {
      area.setLeft((int)this->Internal->Items[leftBin].left());
      area.setRight((int)this->Internal->Items[rightBin].right());
      }
    else
      {
      return;
      }
    }
  else
    {
    const pqChartPixelScale *scale = this->getXAxis()->getPixelValueScale();
    if(!scale->isValid())
      {
      return;
      }

    area.setLeft(scale->getPixel(first.getFirst()));
    area.setRight(scale->getPixel(last.getSecond()));
    }

  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  area.setTop(0);
  area.setBottom(zoomPan->getContentsHeight());
}

void pqHistogramChart::setOptions(const pqHistogramChartOptions &options)
{
  // Copy the new options and repaint the chart.
  *(this->Options) = options;
  emit this->repaintNeeded();
}

bool pqHistogramChart::getAxisRange(const pqChartAxis *axis,
    pqChartValue &min, pqChartValue &max, bool &padMin, bool &padMax) const
{
  if(this->Model && this->Model->getNumberOfBins() > 0)
    {
    if(axis == this->getXAxis())
      {
      this->Model->getRangeX(min, max);
      return true;
      }
    else if(axis == this->getYAxis())
      {
      this->Model->getRangeY(min, max);
      if(axis->getPixelValueScale()->getScaleType() ==
          pqChartPixelScale::Logarithmic)
        {
        if(max < 0)
          {
          if(max.getType() == pqChartValue::IntValue)
            {
            max = (int)0;
            }
          else if(max <= -1)
            {
            max = -0.1;
            max.convertTo(min.getType());
            }
          }
        else if(min > 0)
          {
          if(min.getType() == pqChartValue::IntValue)
            {
            min = (int)0;
            }
          else if(min >= 1)
            {
            min = 0.1;
            min.convertTo(max.getType());
            }
          }
        }
      else
        {
        // The range returned from the model is the data range.
        // Adjust the range to look nice.
        if(max < 0)
          {
          max = (int)0;
          max.convertTo(min.getType());
          }
        else if(min > 0)
          {
          min = (int)0;
          min.convertTo(max.getType());
          }

        // Set up the extra padding parameters for the min/max.
        padMin = true;
        padMax = true;
        if(min == 0)
          {
          padMin = false;
          }
        else if(max == 0)
          {
          padMax = false;
          }
        }

      return true;
      }
    }

  return false;
}

bool pqHistogramChart::isAxisControlPreferred(
    const pqChartAxis *axis) const
{
  return this->Model != 0 && this->Model->getNumberOfBins() > 0 &&
      axis == this->getXAxis();
}

void pqHistogramChart::generateAxisLabels(pqChartAxis *axis)
{
  if(this->isAxisControlPreferred(axis))
    {
    pqChartAxisModel *axisModel = axis->getModel();
    axisModel->startModifyingData();
    axisModel->removeAllLabels();
    pqChartValue min, max;
    for(int i = 0; i < this->Model->getNumberOfBins(); i++)
      {
      this->Model->getBinRange(i, min, max);
      if(i == 0)
        {
        axisModel->addLabel(min);
        }

      axisModel->addLabel(max);
      }

    axisModel->finishModifyingData();
    }
}

void pqHistogramChart::layoutChart(const QRect &area)
{
  if(!this->Model || !this->getChartArea() || !area.isValid())
    {
    return;
    }

  // Make sure the axes are valid.
  const pqChartPixelScale *xScale = this->getXAxis()->getPixelValueScale();
  const pqChartPixelScale *yScale = this->getYAxis()->getPixelValueScale();
  if(!xScale->isValid() || !yScale->isValid())
    {
    return;
    }

  // Save the chart bounds.
  this->Internal->Bounds = area;

  // Set up the contents size.
  this->Internal->Contents = this->Internal->Bounds;
  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  if(zoomPan)
    {
    this->Internal->Contents.setRight(this->Internal->Contents.right() +
        zoomPan->getMaximumXOffset());
    this->Internal->Contents.setBottom(this->Internal->Contents.bottom() +
        zoomPan->getMaximumYOffset());
    }

  // Allocate space for the histogram bars.
  if(this->Internal->Items.size() != this->Model->getNumberOfBins())
    {
    this->Internal->Items.resize(this->Model->getNumberOfBins());
    }

  // Set up the size for each bar in the histogram.
  pqChartValue value, min, max;
  float bottom = yScale->getMinPixel();
  bool reversed = false;
  if(yScale->isZeroInRange())
    {
    pqChartValue zero(0);
    zero.convertTo(yScale->getMaxValue().getType());
    bottom = yScale->getPixelF(zero);
    }
  else
    {
    this->Model->getRangeY(min, max);
    if(max <= 0)
      {
      bottom = yScale->getMaxPixel();
      reversed = true;
      }
    }

  QVector<QRectF>::Iterator iter = this->Internal->Items.begin();
  for(int i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
    {
    this->Model->getBinValue(i, value);
    this->Model->getBinRange(i, min, max);
    iter->setLeft(xScale->getPixelF(min));
    iter->setRight(xScale->getPixelF(max));
    if(reversed || value < 0)
      {
      iter->setTop(bottom);
      iter->setBottom(yScale->getPixelF(value));
      }
    else
      {
      iter->setTop(yScale->getPixelF(value));
      iter->setBottom(bottom);
      }
    }

  this->layoutSelection();
}

void pqHistogramChart::drawBackground(QPainter &painter, const QRect &area)
{
  if(!painter.isActive() || !area.isValid() ||
      !this->Internal->Bounds.isValid())
    {
    return;
    }

  // Translate the painter and the area to paint to contents space.
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  QRect clipArea = area.intersect(this->Internal->Bounds);
  QRect toPaint = area;
  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  if(zoomPan)
    {
    painter.translate(-zoomPan->getXOffset(), -zoomPan->getYOffset());
    toPaint.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    clipArea.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    }

  // Clip the painting area to the chart bounds.
  painter.setClipping(true);
  painter.setClipRect(clipArea);

  // Draw in the selection if there is one.
  QVector<QRectF>::Iterator highlight = this->Internal->Highlights.begin();
  for( ; highlight != this->Internal->Highlights.end(); ++highlight)
    {
    if(highlight->intersects(toPaint))
      {
      painter.fillRect(*highlight, this->Options->getHighlightColor());
      }
    }

  painter.restore();
}

void pqHistogramChart::drawChart(QPainter &painter, const QRect &area)
{
  if(!painter.isActive() || !area.isValid() ||
      !this->Internal->Bounds.isValid())
    {
    return;
    }

  // Translate the painter and the area to paint to contents space.
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  QRect clipArea = area.intersect(this->Internal->Bounds);
  QRect toPaint = area;
  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  if(zoomPan)
    {
    painter.translate(-zoomPan->getXOffset(), -zoomPan->getYOffset());
    toPaint.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    clipArea.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    }

  // Clip the painting area to the chart bounds.
  painter.setClipping(true);
  painter.setClipRect(clipArea);

  // Draw in the histogram bars.
  int i = 0;
  bool areaFound = false;
  int total = this->Model->getNumberOfBins();
  QVector<QRectF>::Iterator highlight = this->Internal->Highlights.begin();
  QVector<QRectF>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter, ++i)
    {
    // Make sure the bounding rectangle is known.
    if(!iter->isValid())
      {
      continue;
      }

    // Check to see if the bar needs to be drawn.
    if(!(iter->left() > toPaint.right() || iter->right() < toPaint.left()))
      {
      areaFound = true;
      if(!(iter->top() > toPaint.bottom() || iter->bottom() < toPaint.top()))
        {
        // First, fill the rectangle. If a bar color scheme is set,
        // get the bar color from the scheme.
        QColor bin = Qt::red;
        if(this->Options->getColorScheme())
          {
          bin = this->Options->getColorScheme()->getColor(i, total);
          }

        // Set the outline color.
        QPen outlinePen = painter.pen();
        outlinePen.setJoinStyle(Qt::MiterJoin);
        outlinePen.setWidth(1);
        if(this->Options->getOutlineStyle() == pqHistogramChartOptions::Darker)
          {
          outlinePen.setColor(bin.dark());
          }
        else
          {
          outlinePen.setColor(Qt::black);
          }

        painter.setPen(outlinePen);
        painter.setBrush(bin);
        QRectF binRect = *iter;
        painter.drawRect(binRect);

        // Set up the painter to draw the highlights.
        if(this->Options->getHighlightStyle() == pqHistogramChartOptions::Fill)
          {
          painter.setBrush(bin.light(170));
          }
        else
          {
          outlinePen.setWidth(2);
          outlinePen.setColor(bin.light(170));
          painter.setPen(outlinePen);
          painter.setBrush(QBrush());
          binRect.adjust(1, 1, 0, 0);
          }

        // Draw in the highlighted portion of the bin rectangle.
        for( ; highlight != this->Internal->Highlights.end(); ++highlight)
          {
          if(iter->right() < highlight->left())
            {
            break;
            }
          else if(iter->left() <= highlight->right())
            {
            painter.save();
            painter.setClipRect(highlight->intersect(iter->adjusted(0, 0, 1, 1)));
            painter.drawRect(binRect);
            painter.restore();
            if(iter->right() <= highlight->right())
              {
              break;
              }
            }
          }
        }
      }
    else if(areaFound)
      {
      break;
      }
    }

  // Draw in the selection outline.
  painter.setPen(QColor(60, 90, 135));
  painter.setBrush(QBrush());
  highlight = this->Internal->Highlights.begin();
  for( ; highlight != this->Internal->Highlights.end(); ++highlight)
    {
    if(highlight->intersects(toPaint))
      {
      painter.drawRect(*highlight);
      }
    }

  painter.restore();
}

void pqHistogramChart::handleModelReset()
{
  if(!this->Model)
    {
    return;
    }

  // Clean up the current chart data and highlights. Make sure the
  // selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();
  this->Internal->Items.clear();
  this->Internal->Highlights.clear();

  // Set up the axis ranges and update the chart layout.
  emit this->rangeChanged();
  emit this->layoutNeeded();

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

void pqHistogramChart::startBinInsertion(int first, int last)
{
  // Notify the selection model of the change. The selection will be
  // adjusted for the changes in this call so it can be layed out
  // when the changed is completed.
  this->InModelChange = true;
  this->Selection->beginInsertBinValues(first, last);
}

void pqHistogramChart::finishBinInsertion()
{
  emit this->layoutNeeded();

  // Close the event for the selection model, which will trigger a
  // selection change signal.
  this->Selection->endInsertBinValues();
  this->InModelChange = false;
}

void pqHistogramChart::startBinRemoval(int first, int last)
{
  // Notify the selection model of the change. The selection will be
  // adjusted for the changes in this call so it can be layed out
  // when the changed is completed.
  this->InModelChange = true;
  this->Selection->beginRemoveBinValues(first, last);
}

void pqHistogramChart::finishBinRemoval()
{
  emit this->layoutNeeded();

  // Close the event for the selection model, which will trigger a
  // selection change signal.
  this->Selection->endRemoveBinValues();
  this->InModelChange = false;
}

void pqHistogramChart::updateHighlights()
{
  if(!this->InModelChange && this->Internal->Contents.isValid())
    {
    this->layoutSelection();
    emit this->repaintNeeded();
    }
}

void pqHistogramChart::layoutSelection()
{
  // Make sure the axes are valid.
  const pqChartPixelScale *xScale = this->getXAxis()->getPixelValueScale();
  if(!xScale->isValid())
    {
    return;
    }

  // Get the selected ranges from the selection model.
  const pqHistogramSelectionList &list = this->Selection->getSelection();

  // Make sure the space allocated for the selection highlight(s)
  // is the same as the space needed.
  if(this->Internal->Highlights.size() != list.size())
    {
    this->Internal->Highlights.resize(list.size());
    }

  QVector<QRectF>::Iterator highlight = this->Internal->Highlights.begin();
  pqHistogramSelectionList::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter, ++highlight)
    {
    // Set up the highlight rectangle for each selection range
    // using the axis scale.
    highlight->setTop(this->Internal->Contents.top());
    highlight->setBottom(this->Internal->Contents.bottom());
    if(iter->getType() == pqHistogramSelection::Value)
      {
      highlight->setLeft(xScale->getPixelF(iter->getFirst()));
      highlight->setRight(xScale->getPixelF(iter->getSecond()));
      }
    else
      {
      // Use the bin rectangles to set the sides.
      highlight->setLeft(this->Internal->Items[iter->getFirst()].left());
      highlight->setRight(this->Internal->Items[iter->getSecond()].right());
      }
    }
}


