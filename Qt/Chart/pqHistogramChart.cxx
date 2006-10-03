/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChart.cxx

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

/*!
 * \file pqHistogramChart.cxx
 *
 * \brief
 *   The pqHistogramChart class is used to display a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 12, 2005
 */

#include "pqHistogramChart.h"

#include "pqChartValue.h"
#include "pqChartAxis.h"
#include "pqHistogramColor.h"
#include "pqHistogramModel.h"
#include "pqHistogramSelection.h"
#include "pqHistogramSelectionModel.h"

#include <QFontMetrics>
#include <QPainter>
#include <QVector>


/// \class pqHistogramChartData
/// \brief
///   The pqHistogramChartData class hides the private data of the
///   pqHistogramChart class.
class pqHistogramChartData
{
public:
  pqHistogramChartData();
  ~pqHistogramChartData() {}

public:
  QVector<QRect> Items;      ///< The list of histogram bars.
  QVector<QRect> Highlights; ///< The list of highlighted ranges.

  /// Stores the default color scheme for the histogram.
  static pqHistogramColor ColorScheme;
};


//----------------------------------------------------------------------------
pqHistogramColor pqHistogramChartData::ColorScheme;

pqHistogramChartData::pqHistogramChartData()
  : Items(), Highlights()
{
}


//----------------------------------------------------------------------------
QColor pqHistogramChart::LightBlue = QColor(125, 165, 230);

pqHistogramChart::pqHistogramChart(QObject *p)
  : QObject(p), Bounds(), Select(LightBlue)
{
  this->Style = pqHistogramChart::Fill;
  this->OutlineType = pqHistogramChart::Darker;
  this->Colors = &pqHistogramChartData::ColorScheme;
  this->Data = new pqHistogramChartData();
  this->Model = 0;
  this->Selection = new pqHistogramSelectionModel(this);
  this->InModelChange = false;

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const pqHistogramSelectionList &)),
      this, SLOT(updateHighlights(const pqHistogramSelectionList &)));
}

pqHistogramChart::~pqHistogramChart()
{
  this->clearData();
  delete this->Data;
  delete this->Selection;
}

void pqHistogramChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis)
{
  this->XAxis = xAxis;
  this->YAxis = yAxis;

  // If the model is set, update the axis ranges from the model.
  if(this->Model)
    {
    this->updateAxisRanges();
    emit this->layoutNeeded();
    }
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
  this->clearData();

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
    this->connect(this->Model, SIGNAL(binValuesReset()),
        this, SLOT(handleModelReset()));
    this->connect(this->Model, SIGNAL(aboutToInsertBinValues(int, int)),
        this, SLOT(startBinInsertion(int, int)));
    this->connect(this->Model, SIGNAL(binValuesInserted()),
        this, SLOT(finishBinInsertion()));
    this->connect(this->Model, SIGNAL(aboutToRemoveBinValues(int, int)),
        this, SLOT(startBinRemoval(int, int)));
    this->connect(this->Model, SIGNAL(binValuesRemoved()),
        this, SLOT(finishBinRemoval()));
    }

  // Set up the axis ranges and update the chart layout.
  this->updateAxisRanges();
  emit this->layoutNeeded();

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

int pqHistogramChart::getBinWidth() const
{
  if(this->Model && this->Model->getNumberOfBins() > 0 &&
      this->Bounds.isValid())
    {
    return this->Bounds.width()/this->Model->getNumberOfBins();
    }

  return 0;
}

int pqHistogramChart::getBinAt(int x, int y, bool entireBin) const
{
  if(this->Bounds.isValid() && this->Bounds.contains(x, y))
    {
    QVector<QRect>::Iterator iter = this->Data->Items.begin();
    for(int i = 0; iter != this->Data->Items.end(); iter++, i++)
      {
      if(entireBin && iter->isValid() && iter->left() < x && x < iter->right())
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
  if(this->Bounds.isValid() && this->XAxis->isValid() &&
      this->Bounds.contains(x, y))
    {
    pqChartValue range = this->XAxis->getValueRange();
    if(range.getType() == pqChartValue::IntValue)
      {
      // Adjust the pick location if the pixel to value ratio is
      // bigger than one. This will help find the closest value.
      int pixel = 0;
      if(range != 0)
        {
        pixel = this->XAxis->getPixelRange() / range;
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

    value = this->XAxis->getValueFor(x);
    return true;
    }

  return false;
}

bool pqHistogramChart::getValueRangeAt(int x, int y,
    pqHistogramSelection &range) const
{
  if(this->Bounds.isValid() && this->XAxis->isValid() &&
    this->Bounds.contains(x, y))
    {
    // Make sure the selection type is 'Value'.
    if(this->Selection->getType() == pqHistogramSelection::Value)
      {
      const pqHistogramSelectionList &list = this->Selection->getSelection();
      pqChartValue diff = this->XAxis->getValueRange();
      if(diff.getType() == pqChartValue::IntValue)
        {
        // Adjust the pick location if the pixel to value ratio is
        // bigger than one. This will help find the closest value.
        int pixel = 0;
        if(diff != 0)
          {
          pixel = this->XAxis->getPixelRange() / diff;
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
      pqChartValue value = this->XAxis->getValueFor(x);
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
    pqHistogramSelectionList &list, bool entireBins) const
{
  if(this->Bounds.isValid() && area.intersects(this->Bounds))
    {
    pqChartValue i((int)0);
    QVector<QRect>::Iterator iter = this->Data->Items.begin();
    for( ; iter != this->Data->Items.end(); ++iter, ++i)
      {
      if(area.right() < iter->left())
        {
        break;
        }

      bool intersection = false;
      if(entireBins && iter->isValid() && iter->left() < area.right() &&
          iter->right() > area.left())
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
  if(!area.isValid() || !this->Bounds.isValid())
    {
    return;
    }

  if(!this->XAxis->isValid() || !area.intersects(this->Bounds))
    {
    return;
    }

  pqChartValue left;
  pqChartValue right;
  QRect i = area.intersect(this->Bounds);
  if(this->getValueAt(i.left(), i.top(), left) &&
      this->getValueAt(i.right(), i.top(), right))
    {
    pqHistogramSelection selection(left, right);
    selection.setType(pqHistogramSelection::Value);
    list.append(selection);
    }
}

void pqHistogramChart::setBinColorScheme(pqHistogramColor *scheme)
{
  if(!scheme && this->Colors == &pqHistogramChartData::ColorScheme)
    {
    return;
    }

  if(this->Colors != scheme)
    {
    if(scheme)
      {
      this->Colors = scheme;
      }
    else
      {
      this->Colors = &pqHistogramChartData::ColorScheme;
      }

    emit this->repaintNeeded();
    }
}

void pqHistogramChart::setBinHighlightStyle(HighlightStyle style)
{
  if(this->Style != style)
    {
    this->Style = style;
    emit this->repaintNeeded();
    }
}

void pqHistogramChart::setBinOutlineStyle(OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit repaintNeeded();
    }
}

void pqHistogramChart::layoutChart()
{
  if(!this->Model || !this->XAxis || !this->YAxis)
    {
    return;
    }

  // Make sure the axes are valid.
  if(!this->XAxis->isValid() || !this->YAxis->isValid())
    {
    return;
    }

  // Set up the chart area based on the remaining space.
  this->Bounds.setTop(this->YAxis->getMaxPixel());
  this->Bounds.setLeft(this->XAxis->getMinPixel());
  this->Bounds.setRight(this->XAxis->getMaxPixel());
  this->Bounds.setBottom(this->YAxis->getMinPixel());

  // Allocate space for the histogram bars.
  if(this->Data->Items.size() != this->Model->getNumberOfBins())
    {
    this->Data->Items.resize(this->Model->getNumberOfBins());
    }

  // Set up the size for each bar in the histogram.
  int bottom = this->YAxis->getMinPixel();
  bool reversed = false;
  if(this->YAxis->isZeroInRange())
    {
    pqChartValue zero(0);
    zero.convertTo(this->YAxis->getMaxValue().getType());
    bottom = this->YAxis->getPixelFor(zero);
    }
  else if(this->YAxis->getMaxValue() <= 0)
    {
    bottom = this->YAxis->getMaxPixel();
    reversed = true;
    }

  pqChartValue value;
  int total = this->XAxis->getNumberShowing();
  QVector<QRect>::Iterator iter = this->Data->Items.begin();
  for(int i = 0; iter != this->Data->Items.end() && i < total; ++iter, ++i)
    {
    this->Model->getBinValue(i, value);
    iter->setLeft(this->XAxis->getPixelForIndex(i));
    iter->setRight(this->XAxis->getPixelForIndex(i + 1));
    if(reversed || value < 0)
      {
      iter->setTop(bottom);
      iter->setBottom(this->YAxis->getPixelFor(value));
      }
    else
      {
      iter->setTop(this->YAxis->getPixelFor(value));
      iter->setBottom(bottom);
      }
    }

  this->layoutSelection();
}

void pqHistogramChart::drawBackground(QPainter *p, const QRect &area)
{
  if(!p || !p->isActive() || !area.isValid() || !this->Bounds.isValid())
    {
    return;
    }

  // Draw in the selection if there is one.
  QVector<QRect>::Iterator highlight = this->Data->Highlights.begin();
  for( ; highlight != this->Data->Highlights.end(); ++highlight)
    {
    if(highlight->intersects(area))
      {
      p->fillRect(*highlight, this->Select);
      }
    }
}

void pqHistogramChart::drawChart(QPainter *p, const QRect &area)
{
  if(!p || !p->isActive() || !area.isValid() || !this->Bounds.isValid())
    {
    return;
    }

  // Draw in the histogram bars.
  int i = 0;
  bool areaFound = false;
  int total = this->XAxis->getNumberShowing();
  QVector<QRect>::Iterator highlight = this->Data->Highlights.begin();
  QVector<QRect>::Iterator iter = this->Data->Items.begin();
  for( ; iter != this->Data->Items.end() && i < total; ++iter, ++i)
    {
    // Make sure the bounding rectangle is known.
    if(!iter->isValid())
      {
      continue;
      }

    // Check to see if the bar needs to be drawn.
    if(!(iter->left() > area.right() || iter->right() < area.left()))
      {
      areaFound = true;
      if(!(iter->top() > area.bottom() || iter->bottom() < area.top()))
        {
        // First, fill the rectangle. If a bar color scheme is set,
        // get the bar color from the scheme.
        QColor bin = Qt::red;
        if(this->Colors)
          {
          bin = this->Colors->getColor(i, this->Model->getNumberOfBins());
          }

        p->fillRect(iter->x(), iter->y(), iter->width() - 1,
            iter->height() - 1, bin);

        // Draw in the highlighted portion of the bar if it is selected
        // and fill highlighting is being used.
        if(this->Style == pqHistogramChart::Fill)
          {
          for( ; highlight != this->Data->Highlights.end(); ++highlight)
            {
            if(iter->right() < highlight->left())
              {
              break;
              }
            else if(iter->left() <= highlight->right())
              {
              p->fillRect(iter->intersect(*highlight), bin.light(170));
              if(iter->right() <= highlight->right())
                {
                break;
                }
              }
            }
          }

        // Draw in the outline last to make sure it is on top.
        if(this->OutlineType == pqHistogramChart::Darker)
          {
          p->setPen(bin.dark());
          }
        else
          {
          p->setPen(Qt::black);
          }

        p->drawRect(iter->x(), iter->y(), iter->width() - 1,
            iter->height() - 1);

        // If the bar is selected and outline highlighting is used, draw
        // a lighter and thicker outline around the bar.
        if(this->Style == pqHistogramChart::Outline)
          {
          for( ; highlight != this->Data->Highlights.end(); ++highlight)
            {
            if(iter->right() < highlight->left())
              {
              break;
              }
            else if(iter->left() <= highlight->right())
              {
              p->setPen(bin.light(170));
              QRect inter = iter->intersect(*highlight);
              inter.setWidth(inter.width() - 1);
              inter.setHeight(inter.height() - 1);
              p->drawRect(inter);
              inter.translate(1, 1);
              inter.setWidth(inter.width() - 2);
              inter.setHeight(inter.height() - 2);
              p->drawRect(inter);
              if(iter->right() <= highlight->right())
                {
                break;
                }
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
  p->setPen(QColor(60, 90, 135));
  highlight = this->Data->Highlights.begin();
  for( ; highlight != this->Data->Highlights.end(); ++highlight)
    {
    if(highlight->intersects(area))
      {
      p->drawRect(highlight->x(), highlight->y(), highlight->width() - 1,
          highlight->height() - 1);
      }
    }
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
  this->clearData();

  // Set up the axis ranges and update the chart layout.
  this->updateAxisRanges();
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
  // Adjust the y-axis range to account for the new values. Inserting
  // any number of bins will require all the bin sizes to be
  // re-calculated.
  this->updateYAxisRange();
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
  // Removing bins can change the y-axis range. Removing any number
  // of bins will require all the bin sizes to be re-calculated.
  this->updateYAxisRange();
  emit this->layoutNeeded();

  // Close the event for the selection model, which will trigger a
  // selection change signal.
  this->Selection->endRemoveBinValues();
  this->InModelChange = false;
}

void pqHistogramChart::updateHighlights(const pqHistogramSelectionList &)
{
  if(!this->InModelChange)
    {
    this->layoutSelection();
    emit this->repaintNeeded();
    }
}

void pqHistogramChart::layoutSelection()
{
  if(this->Bounds.isValid())
    {
    // Get the selected ranges from the selection model.
    const pqHistogramSelectionList &list = this->Selection->getSelection();

    // Make sure the space allocated for the selection highlight(s)
    // is the same as the space needed.
    if(this->Data->Highlights.size() != list.size())
      {
      this->Data->Highlights.resize(list.size());
      }

    QVector<QRect>::Iterator highlight = this->Data->Highlights.begin();
    pqHistogramSelectionList::ConstIterator iter = list.begin();
    for( ; iter != list.end(); ++iter, ++highlight)
      {
      // Set up the highlight rectangle for each selection range
      // using the axis scale.
      highlight->setTop(this->Bounds.top());
      highlight->setBottom(this->Bounds.bottom());
      if(iter->getType() == pqHistogramSelection::Value)
        {
        highlight->setLeft(this->XAxis->getPixelFor(iter->getFirst()));
        highlight->setRight(this->XAxis->getPixelFor(iter->getSecond()));
        }
      else
        {
        highlight->setLeft(this->XAxis->getPixelForIndex(iter->getFirst()));
        highlight->setRight(this->XAxis->getPixelForIndex(
            iter->getSecond() + 1));
        }
      }
    }
}

void pqHistogramChart::updateAxisRanges()
{
  this->updateXAxisRange();
  this->updateYAxisRange();
}

void pqHistogramChart::updateXAxisRange()
{
  if(!this->XAxis)
    {
    return;
    }

  int intervals = 0;
  pqChartValue xMin, xMax;
  if(this->Model)
    {
    this->Model->getRangeX(xMin, xMax);
    intervals = this->Model->getNumberOfBins();
    }

  this->XAxis->blockSignals(true);
  this->XAxis->setValueRange(xMin, xMax);
  this->XAxis->setNumberOfIntervals(intervals);
  this->XAxis->blockSignals(false);
}

void pqHistogramChart::updateYAxisRange()
{
  // Don't set the y-axis range when using a fixed-interval layout.
  if(!this->YAxis || this->YAxis->getLayoutType() == pqChartAxis::FixedInterval)
    {
    return;
    }

  pqChartValue yMin, yMax;
  if(this->Model)
    {
    // Adjust the y-axis range for the histogram.
    this->Model->getRangeY(yMin, yMax);
    if(this->YAxis->getScaleType() == pqChartAxis::Logarithmic)
      {
      if(yMax < 0)
        {
        if(yMax.getType() == pqChartValue::IntValue)
          {
          yMax = (int)0;
          }
        else if(yMax <= -1)
          {
          yMax = -0.1;
          yMax.convertTo(yMin.getType());
          }
        }
      else if(yMin > 0)
        {
        if(yMin.getType() == pqChartValue::IntValue)
          {
          yMin = (int)0;
          }
        else if(yMin >= 1)
          {
          yMin = 0.1;
          yMin.convertTo(yMax.getType());
          }
        }
      }
    else
      {
      // The range returned from the model is the data range. Adjust
      // the range to look nice.
      if(yMax < 0)
        {
        yMax = (int)0;
        yMax.convertTo(yMin.getType());
        }
      else if(yMin > 0)
        {
        yMin = (int)0;
        yMin.convertTo(yMax.getType());
        }

      // Set up the extra padding parameters for the min/max.
      if(yMin == 0)
        {
        this->YAxis->setExtraMaxPadding(true);
        this->YAxis->setExtraMinPadding(false);
        }
      else if(yMax == 0)
        {
        this->YAxis->setExtraMaxPadding(false);
        this->YAxis->setExtraMinPadding(true);
        }
      else
        {
        this->YAxis->setExtraMaxPadding(true);
        this->YAxis->setExtraMinPadding(true);
        }
      }
    }

  this->YAxis->blockSignals(true);
  this->YAxis->setValueRange(yMin, yMax);
  this->YAxis->blockSignals(false);
}

void pqHistogramChart::clearData()
{
  this->Data->Items.clear();
  this->Data->Highlights.clear();
}


