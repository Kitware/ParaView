/*=========================================================================

   Program: ParaView
   Module:    pqChartArea.cxx

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

/// \file pqChartArea.cxx
/// \date 11/20/2006

#include "pqChartArea.h"

#include "pqChartAxisLayer.h"
#include "pqChartAxisModel.h"
#include "pqChartAxisOptions.h"
#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartGridLayer.h"
#include "pqChartInteractor.h"
#include "pqChartMouseBox.h"
#include "pqChartLayer.h"

#include <QCursor>
#include <QEvent>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPrinter>
#include <QRect>

#define TOO_SMALL_WIDTH 40
#define TOO_SMALL_HEIGHT 30


class pqChartAreaAxisItem
{
public:
  pqChartAreaAxisItem();
  ~pqChartAreaAxisItem() {}

  pqChartArea::AxisBehavior Behavior;
  bool Modified;
};


class pqChartAreaInternal
{
public:
  enum AxisIndex
    {
    LeftIndex = 0,
    TopIndex,
    RightIndex,
    BottomIndex,
    AxisCount
    };

public:
  pqChartAreaInternal();
  ~pqChartAreaInternal();

  int convertEnum(pqChartAxis::AxisLocation location) const;

  QList<pqChartLayer *> Layers; ///< Stores the chart layers.
  pqChartAreaAxisItem *Option;  ///< Stores the axis behaviors.
  pqChartAxis **Axis;           ///< Stores the axis objects.
  bool RangeChanged;            ///< True if the range has changed.
  bool InResize;                ///< True if the widget is resizing.
};


//----------------------------------------------------------------------------
pqChartAreaAxisItem::pqChartAreaAxisItem()
{
  this->Behavior = pqChartArea::ChartSelect;
  this->Modified = true;
}


//----------------------------------------------------------------------------
pqChartAreaInternal::pqChartAreaInternal()
  : Layers()
{
  this->Option = new pqChartAreaAxisItem[pqChartAreaInternal::AxisCount];
  this->Axis = new pqChartAxis *[pqChartAreaInternal::AxisCount];
  this->RangeChanged = false;
  this->InResize = false;

  // Initialize the axis pointers.
  for(int i = 0; i < pqChartAreaInternal::AxisCount; i++)
    {
    this->Axis[i] = 0;
    }
}

pqChartAreaInternal::~pqChartAreaInternal()
{
  // The axis objects will get deleted up by Qt's parent-child cleanup.
  delete [] this->Option;
  delete [] this->Axis;
}

int pqChartAreaInternal::convertEnum(
    pqChartAxis::AxisLocation location) const
{
  switch(location)
    {
    case pqChartAxis::Left:
      {
      return pqChartAreaInternal::LeftIndex;
      }
    case pqChartAxis::Top:
      {
      return pqChartAreaInternal::TopIndex;
      }
    case pqChartAxis::Right:
      {
      return pqChartAreaInternal::RightIndex;
      }
    case pqChartAxis::Bottom:
      {
      return pqChartAreaInternal::BottomIndex;
      }
    }

  return -1;
}


//----------------------------------------------------------------------------
pqChartArea::pqChartArea(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->Internal = new pqChartAreaInternal();
  this->GridLayer = new pqChartGridLayer(this);
  this->AxisLayer = new pqChartAxisLayer(this);
  this->Contents = new pqChartContentsSpace(this);
  this->MouseBox = new pqChartMouseBox();
  this->Interactor = 0;

  this->GridLayer->setObjectName("GridLayer");
  this->AxisLayer->setObjectName("AxisLayer");
  this->Contents->setObjectName("ContentsSpace");

  // Add the grid layer and axis layer to the list.
  this->Internal->Layers.append(this->GridLayer);
  this->Internal->Layers.append(this->AxisLayer);

  // Set the default size and focus policy.
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  this->setFocusPolicy(Qt::WheelFocus);

  // Listen for zoom/pan changes.
  this->connect(this->Contents, SIGNAL(xOffsetChanged(int)),
      this, SLOT(update()));
  this->connect(this->Contents, SIGNAL(yOffsetChanged(int)),
      this, SLOT(update()));
  this->connect(this->Contents, SIGNAL(maximumChanged(int, int)),
      this, SLOT(handleZoomChange()));
}

pqChartArea::~pqChartArea()
{
  delete this->Internal;
  delete this->MouseBox;
}

QSize pqChartArea::sizeHint() const
{
  // TODO: Base the size hint on the chart size requirements.
  return QSize(100, 100);
}

void pqChartArea::createAxis(pqChartAxis::AxisLocation location)
{
  pqChartAxis *axis = this->getAxis(location);
  if(!axis)
    {
    int index = this->Internal->convertEnum(location);
    if(index == -1)
      {
      return;
      }

    // Create the axis and a model for the axis.
    pqChartAxis *across = 0;
    axis = new pqChartAxis(location, this);
    this->Internal->Axis[index] = axis;
    pqChartAxisModel *model = new pqChartAxisModel(this);
    axis->setModel(model);
    axis->setContentsScpace(this->Contents);
    if(location == pqChartAxis::Top || location == pqChartAxis::Bottom)
      {
      // Set the neighbors. Make sure the neighbors' neighbors are set.
      axis->setNeigbors(this->Internal->Axis[pqChartAreaInternal::LeftIndex],
          this->Internal->Axis[pqChartAreaInternal::RightIndex]);
      if(this->Internal->Axis[pqChartAreaInternal::LeftIndex])
        {
        this->Internal->Axis[pqChartAreaInternal::LeftIndex]->setNeigbors(
            this->Internal->Axis[pqChartAreaInternal::BottomIndex],
            this->Internal->Axis[pqChartAreaInternal::TopIndex]);
        }

      if(this->Internal->Axis[pqChartAreaInternal::RightIndex])
        {
        this->Internal->Axis[pqChartAreaInternal::RightIndex]->setNeigbors(
            this->Internal->Axis[pqChartAreaInternal::BottomIndex],
            this->Internal->Axis[pqChartAreaInternal::TopIndex]);
        }

      if(location == pqChartAxis::Top)
        {
        axis->setObjectName("TopAxis");
        model->setObjectName("TopAxisModel");
        across = this->Internal->Axis[pqChartAreaInternal::BottomIndex];

        // Add the axis to the grid and axis layer.
        this->GridLayer->setTopAxis(axis);
        this->AxisLayer->setTopAxis(axis);
        }
      else
        {
        axis->setObjectName("BottomAxis");
        model->setObjectName("BottomAxisModel");
        across = this->Internal->Axis[pqChartAreaInternal::TopIndex];

        // Add the axis to the grid and axis layer.
        this->GridLayer->setBottomAxis(axis);
        this->AxisLayer->setBottomAxis(axis);
        }
      }
    else
      {
      // Set the neighbors. Make sure the neighbors' neighbors are set.
      axis->setNeigbors(this->Internal->Axis[pqChartAreaInternal::BottomIndex],
          this->Internal->Axis[pqChartAreaInternal::TopIndex]);
      if(this->Internal->Axis[pqChartAreaInternal::TopIndex])
        {
        this->Internal->Axis[pqChartAreaInternal::TopIndex]->setNeigbors(
            this->Internal->Axis[pqChartAreaInternal::LeftIndex],
            this->Internal->Axis[pqChartAreaInternal::RightIndex]);
        }

      if(this->Internal->Axis[pqChartAreaInternal::BottomIndex])
        {
        this->Internal->Axis[pqChartAreaInternal::BottomIndex]->setNeigbors(
            this->Internal->Axis[pqChartAreaInternal::LeftIndex],
            this->Internal->Axis[pqChartAreaInternal::RightIndex]);
        }

      if(location == pqChartAxis::Left)
        {
        axis->setObjectName("LeftAxis");
        model->setObjectName("LeftAxisModel");
        across = this->Internal->Axis[pqChartAreaInternal::RightIndex];

        // Add the axis to the grid and axis layer.
        this->GridLayer->setLeftAxis(axis);
        this->AxisLayer->setLeftAxis(axis);
        }
      else
        {
        axis->setObjectName("RightAxis");
        model->setObjectName("RightAxisModel");
        across = this->Internal->Axis[pqChartAreaInternal::LeftIndex];

        // Add the axis to the grid and axis layer.
        this->GridLayer->setRightAxis(axis);
        this->AxisLayer->setRightAxis(axis);
        }
      }

    // Set the parallel axis.
    if(across)
      {
      axis->setParallelAxis(across);
      across->setParallelAxis(axis);
      }

    // Listen to the axis update signals.
    this->connect(axis, SIGNAL(layoutNeeded()), this, SLOT(layoutChart()));
    this->connect(axis, SIGNAL(repaintNeeded()), this, SLOT(update()));
    }
}

pqChartAxis *pqChartArea::getAxis(pqChartAxis::AxisLocation location) const
{
  int index = this->Internal->convertEnum(location);
  if(index != -1)
    {
    return this->Internal->Axis[index];
    }

  return 0;
}

pqChartArea::AxisBehavior pqChartArea::getAxisBehavior(
    pqChartAxis::AxisLocation location) const
{
  int index = this->Internal->convertEnum(location);
  if(index != -1)
    {
    return this->Internal->Option[index].Behavior;
    }

  return pqChartArea::ChartSelect;
}

void pqChartArea::setAxisBehavior(pqChartAxis::AxisLocation location,
    pqChartArea::AxisBehavior behavior)
{
  int index = this->Internal->convertEnum(location);
  if(index != -1 && this->Internal->Option[index].Behavior != behavior)
    {
    this->Internal->Option[index].Behavior = behavior;
    this->Internal->Option[index].Modified = true;
    }
}

void pqChartArea::addLayer(pqChartLayer *chart)
{
  this->insertLayer(this->Internal->Layers.size(), chart);
}

void pqChartArea::insertLayer(int index, pqChartLayer *chart)
{
  // Make sure the chart isn't in the list already.
  if(this->Internal->Layers.indexOf(chart) != -1)
    {
    return;
    }

  // Make sure the index is valid.
  if(index < 0)
    {
    index = 0;
    }
  else if(index > this->Internal->Layers.size())
    {
    index = this->Internal->Layers.size();
    }

  // Add the chart to the list of layers.
  if(index == this->Internal->Layers.size())
    {
    this->Internal->Layers.append(chart);
    }
  else
    {
    this->Internal->Layers.insert(index, chart);
    }

  // Set the contents space handler.
  chart->setContentsSpace(this->Contents);

  // Listen for the chart update signals.
  this->connect(chart, SIGNAL(layoutNeeded()), this, SLOT(layoutChart()));
  this->connect(chart, SIGNAL(repaintNeeded()), this, SLOT(update()));
  this->connect(chart, SIGNAL(rangeChanged()),
      this, SLOT(handleChartRangeChange()));
  this->Internal->RangeChanged = true;
}

void pqChartArea::removeLayer(pqChartLayer *chart)
{
  // Get the index for the chart later. If the chart is not in the
  // layer list, ignore the request.
  int index = this->Internal->Layers.indexOf(chart);
  if(index == -1)
    {
    return;
    }

  // Remove the chart layer from the list.
  this->Internal->Layers.removeAt(index);
  chart->setContentsSpace(0);
  this->disconnect(chart, 0, this, 0);
  this->Internal->RangeChanged = true;
}

int pqChartArea::getGridLayerIndex() const
{
  return this->Internal->Layers.indexOf(this->GridLayer);
}

int pqChartArea::getAxisLayerIndex() const
{
  return this->Internal->Layers.indexOf(this->AxisLayer);
}

void pqChartArea::setInteractor(pqChartInteractor *interactor)
{
  if(this->Interactor)
    {
    this->Interactor->setContentsSpace(0);
    this->Interactor->setMouseBox(0);
    this->disconnect(this->Interactor, 0, this, 0);
    }

  this->Interactor = interactor;
  if(this->Interactor)
    {
    this->Interactor->setContentsSpace(this->Contents);
    this->Interactor->setMouseBox(this->MouseBox);
    this->connect(this->Interactor, SIGNAL(repaintNeeded()),
        this, SLOT(update()));
    this->connect(this->Interactor, SIGNAL(repaintNeeded(const QRect &)),
        this, SLOT(updateArea(const QRect &)));
    this->connect(
        this->Interactor, SIGNAL(cursorChangeRequested(const QCursor &)),
        this, SLOT(changeCursor(const QCursor &)));
    }
}

void pqChartArea::printChart(QPrinter &printer)
{
  // Set up the painter for the printer.
  QSize viewportSize = this->size();
  viewportSize.scale(printer.pageRect().size(), Qt::KeepAspectRatio);

  QPainter painter(&printer);
  painter.setWindow(this->rect());
  painter.setViewport(QRect(QPoint(0, 0), viewportSize));

  this->drawChart(painter, this->rect());
}

void pqChartArea::drawChart(QPainter &painter, const QRect &area)
{
  // Draw the background first.
  QList<pqChartLayer *>::Iterator layer = this->Internal->Layers.begin();
  for( ; layer != this->Internal->Layers.end(); ++layer)
    {
    (*layer)->drawBackground(painter, area);
    }

  // Loop through the layers to draw the combined chart.
  layer = this->Internal->Layers.begin();
  for( ; layer != this->Internal->Layers.end(); ++layer)
    {
    (*layer)->drawChart(painter, area);
    }
}

void pqChartArea::layoutChart()
{
  // If the axis layout behavior is ChartSelect, the axis could be fit
  // to the data range or determined by a chart. If the axis is data
  // driven, give the chart a chance to generate the axis labels.
  int i = 0;
  QList<pqChartLayer *>::Iterator layer;
  for( ; i < pqChartAreaInternal::AxisCount; i++)
    {
    if(this->Internal->Axis[i] == 0)
      {
      continue;
      }

    if((this->Internal->RangeChanged || this->Internal->Option[i].Modified) &&
        this->Internal->Option[i].Behavior == pqChartArea::ChartSelect)
      {
      // See if any of the layers can generate the axis labels. If
      // more than one wants to generate the labels, use a best fit
      // instead.
      pqChartLayer *generator = 0;
      layer = this->Internal->Layers.begin();
      for( ; layer != this->Internal->Layers.end(); ++layer)
        {
        if((*layer)->isAxisControlPreferred(this->Internal->Axis[i]))
          {
          if(generator)
            {
            generator = 0;
            break;
            }
          else
            {
            generator = *layer;
            }
          }
        }

      this->Internal->Axis[i]->setBestFitGenerated(generator == 0);
      if(generator)
        {
        // Block the signals from the axis while it is being modified
        // to prevent recursion.
        this->Internal->Axis[i]->blockSignals(true);
        generator->generateAxisLabels(this->Internal->Axis[i]);
        this->Internal->Axis[i]->blockSignals(false);
        }
      else
        {
        pqChartValue min, max;
        pqChartValue chartMin, chartMax;
        bool firstFound = false;
        bool useMinPadding = false;
        bool useMaxPadding = false;
        layer = this->Internal->Layers.begin();
        for( ; layer != this->Internal->Layers.end(); ++layer)
          {
          bool padMax = false, padMin = false;
          if((*layer)->getAxisRange(
              this->Internal->Axis[i], chartMin, chartMax, padMin, padMax))
            {
            // See if the layer prefers axis padding.
            if(!useMinPadding)
              {
              useMinPadding = padMin;
              }

            if(!useMaxPadding)
              {
              useMaxPadding = padMax;
              }

            // Use the layer's axis range to set the overall range.
            if(firstFound)
              {
              if(chartMin < min)
                {
                min = chartMin;
                }

              if(chartMax > max)
                {
                max = chartMax;
                }
              }
            else
              {
              firstFound = true;
              min = chartMin;
              max = chartMax;
              }
            }
          }

        this->Internal->Axis[i]->setDataAvailable(firstFound);
        this->Internal->Axis[i]->setExtraMinPadding(useMinPadding);
        this->Internal->Axis[i]->setExtraMaxPadding(useMaxPadding);
        this->Internal->Axis[i]->setBestFitRange(min, max);
        }
      }
    else if(this->Internal->Option[i].Modified)
      {
      this->Internal->Axis[i]->setBestFitGenerated(
          this->Internal->Option[i].Behavior == pqChartArea::BestFit);
      }

    this->Internal->Option[i].Modified = false;
    }

  this->Internal->RangeChanged = false;

  // Make sure there is enough vertical space. The top and bottom axes
  // know their preferred size before layout.
  QRect bounds = this->rect();
  int available = 0;
  int fontHeight = 0;
  int index = pqChartAreaInternal::LeftIndex;
  if(this->Internal->Axis[index])
    {
    fontHeight = this->Internal->Axis[index]->getFontHeight();
    }

  index = pqChartAreaInternal::RightIndex;
  if(this->Internal->Axis[index] &&
      this->Internal->Axis[index]->getFontHeight() > fontHeight)
    {
    fontHeight = this->Internal->Axis[index]->getFontHeight();
    }

  fontHeight /= 2;
  int space = 0;
  index = pqChartAreaInternal::TopIndex;
  available = fontHeight;
  if(this->Internal->Axis[index])
    {
    space = this->Internal->Axis[index]->getPreferredSpace();
    if(space > fontHeight)
      {
      available = space;
      }
    }

  index = pqChartAreaInternal::BottomIndex;
  if(this->Internal->Axis[index])
    {
    space = this->Internal->Axis[index]->getPreferredSpace();
    available += space > fontHeight ? space : fontHeight;
    }
  else
    {
    available += fontHeight;
    }

  // Set the 'too small' flag on each of the axis objects.
  bool tooSmall = bounds.height() - available < TOO_SMALL_HEIGHT;
  for(i = 0; i < pqChartAreaInternal::AxisCount; i++)
    {
    if(this->Internal->Axis[i])
      {
      this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
      }
    }

  // Layout the left and right axes first.
  if(this->Internal->Axis[pqChartAreaInternal::LeftIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::LeftIndex]->layoutAxis(bounds);
    }

  if(this->Internal->Axis[pqChartAreaInternal::RightIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::RightIndex]->layoutAxis(bounds);
    }

  QRect axisBounds;
  if(!tooSmall)
    {
    // Make sure there is enough horizontal space.
    available = bounds.width();
    index = pqChartAreaInternal::LeftIndex;
    if(this->Internal->Axis[index])
      {
      this->Internal->Axis[index]->getBounds(axisBounds);
      available -= axisBounds.width();
      }

    index = pqChartAreaInternal::RightIndex;
    if(this->Internal->Axis[index])
      {
      this->Internal->Axis[index]->getBounds(axisBounds);
      available -= axisBounds.width();
      }

    tooSmall = available < TOO_SMALL_WIDTH;
    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout the left and right axes.
      for(i = 0; i < pqChartAreaInternal::AxisCount; i++)
        {
        if(this->Internal->Axis[i])
          {
          this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
          }
        }

      index = pqChartAreaInternal::LeftIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }

      index = pqChartAreaInternal::RightIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }
      }
    }

  // Layout the top and bottom axes. They need size from the left and
  // right axes layout.
  if(this->Internal->Axis[pqChartAreaInternal::TopIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::TopIndex]->layoutAxis(bounds);
    }

  if(this->Internal->Axis[pqChartAreaInternal::BottomIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::BottomIndex]->layoutAxis(bounds);
    if(this->Internal->Axis[pqChartAreaInternal::TopIndex])
      {
      // The top and bottom axes should have the same width. The top
      // axis may need to be layed out again to account for the width
      // of the bottom axis labels.
      index = pqChartAreaInternal::BottomIndex;
      this->Internal->Axis[index]->getBounds(axisBounds);
      int bottomWidth = axisBounds.width();
      index = pqChartAreaInternal::TopIndex;
      this->Internal->Axis[index]->getBounds(axisBounds);
      if(bottomWidth != axisBounds.width())
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }
      }
    }

  if(!tooSmall)
    {
    // Check the horizontal space using the bounds from the top and
    // bottom axes.
    if(this->Internal->Axis[pqChartAreaInternal::TopIndex])
      {
      index = pqChartAreaInternal::TopIndex;
      this->Internal->Axis[index]->getBounds(axisBounds);
      tooSmall = axisBounds.width() < TOO_SMALL_WIDTH;
      }
    else if(this->Internal->Axis[pqChartAreaInternal::BottomIndex])
      {
      index = pqChartAreaInternal::BottomIndex;
      this->Internal->Axis[index]->getBounds(axisBounds);
      tooSmall = axisBounds.width() < TOO_SMALL_WIDTH;
      }

    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout all of the axes.
      for(i = 0; i < pqChartAreaInternal::AxisCount; i++)
        {
        if(this->Internal->Axis[i])
          {
          this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
          }
        }

      index = pqChartAreaInternal::LeftIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }

      index = pqChartAreaInternal::RightIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }

      index = pqChartAreaInternal::TopIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }

      index = pqChartAreaInternal::BottomIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->layoutAxis(bounds);
        }
      }
    else
      {
      // Adjust the size of the left and right axes. The top and bottom
      // axes may have needed more space.
      index = pqChartAreaInternal::LeftIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->adjustAxisLayout();
        }

      index = pqChartAreaInternal::RightIndex;
      if(this->Internal->Axis[index])
        {
        this->Internal->Axis[index]->adjustAxisLayout();
        }
      }
    }

  // Calculate the area inside the axes.
  QRect chartBounds = bounds;
  if(this->Internal->Axis[pqChartAreaInternal::LeftIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::LeftIndex]->getBounds(bounds);
    chartBounds.setLeft(bounds.right());
    chartBounds.setTop(bounds.top());
    chartBounds.setBottom(bounds.bottom());
    }

  if(this->Internal->Axis[pqChartAreaInternal::TopIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::TopIndex]->getBounds(bounds);
    chartBounds.setLeft(bounds.left());
    chartBounds.setTop(bounds.bottom());
    chartBounds.setRight(bounds.right());
    }

  if(this->Internal->Axis[pqChartAreaInternal::RightIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::RightIndex]->getBounds(bounds);
    chartBounds.setTop(bounds.top());
    chartBounds.setRight(bounds.left());
    chartBounds.setBottom(bounds.bottom());
    }

  if(this->Internal->Axis[pqChartAreaInternal::BottomIndex])
    {
    this->Internal->Axis[pqChartAreaInternal::BottomIndex]->getBounds(bounds);
    chartBounds.setLeft(bounds.left());
    chartBounds.setRight(bounds.right());
    chartBounds.setBottom(bounds.top());
    }

  // Layout the chart layers using the calculated bounds.
  this->Contents->setChartLayerBounds(chartBounds);
  layer = this->Internal->Layers.begin();
  for( ; layer != this->Internal->Layers.end(); ++layer)
    {
    (*layer)->layoutChart(chartBounds);
    }

  this->update();
}

bool pqChartArea::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    // Set the font for each of the axes. Ignore the layout request
    // signals so all the axes can be updated before relayout.
    for(int i = 0; i < pqChartAreaInternal::AxisCount; i++)
      {
      if(this->Internal->Axis[i])
        {
        this->Internal->Axis[i]->blockSignals(true);
        this->Internal->Axis[i]->getOptions()->setLabelFont(this->font());
        this->Internal->Axis[i]->blockSignals(false);
        }
      }

    // Layout the chart area.
    this->layoutChart();
    }
  // TODO
  //else if(e->type() == QEvent::ContextMenu && this->Internal->SkipContextMenu)
  //  {
  //  this->Internal->SkipContextMenu = false;
  //  e->accept();
  //  return true;
  //  }
  //else if(e->type() == QEvent::ToolTip)
  //  {
  //  this->LineChart->showTooltip(static_cast<QHelpEvent*>(e));
  //  }

  return QWidget::event(e);
}

void pqChartArea::resizeEvent(QResizeEvent *e)
{
  this->Internal->InResize = true;
  this->Contents->setChartSize(e->size().width(), e->size().height());
  this->layoutChart();
  this->Internal->InResize = false;
}

void pqChartArea::paintEvent(QPaintEvent *e)
{
  QRect area = e->rect();
  if(!area.isValid())
    {
    return;
    }

  QPainter painter(this);
  if(!painter.isActive())
    {
    return;
    }

  // Draw the chart layers.
  this->drawChart(painter, area);

  // Draw the mouse box if it is valid.
  if(this->MouseBox->isValid())
    {
    this->MouseBox->getPaintRectangle(area);
    this->Contents->translateFromContents(area);
    painter.setPen(Qt::black);
    painter.setPen(Qt::DotLine);
    painter.drawRect(area);
    }
}

void pqChartArea::keyPressEvent(QKeyEvent *e)
{
  bool handled = false;
  if(this->Interactor)
    {
    handled = this->Interactor->keyPressEvent(e);
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    QWidget::keyPressEvent(e);
    }
}

void pqChartArea::mousePressEvent(QMouseEvent *e)
{
  // Get the current mouse position and convert it to contents coords.
  QPoint point = e->pos();
  this->Contents->translateToContents(point);

  // Save the necessary coordinates.
  this->MouseBox->setStartingPosition(point);

  // Let the interactor handle the rest of the event.
  if(this->Interactor)
    {
    this->Interactor->mousePressEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void pqChartArea::mouseMoveEvent(QMouseEvent *e)
{
  if(this->Interactor)
    {
    this->Interactor->mouseMoveEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void pqChartArea::mouseReleaseEvent(QMouseEvent *e)
{
  if(this->Interactor)
    {
    this->Interactor->mouseReleaseEvent(e);
    }
  else
    {
    e->ignore();
    }

  this->MouseBox->resetRectangle();
}

void pqChartArea::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(this->Interactor)
    {
    this->Interactor->mouseDoubleClickEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void pqChartArea::wheelEvent(QWheelEvent *e)
{
  if(this->Interactor)
    {
    this->Interactor->wheelEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void pqChartArea::handleChartRangeChange()
{
  this->Internal->RangeChanged = true;
}

void pqChartArea::handleZoomChange()
{
  if(!this->Internal->InResize)
    {
    this->layoutChart();
    }
}

void pqChartArea::changeCursor(const QCursor &newCursor)
{
  this->setCursor(newCursor);
}

void pqChartArea::updateArea(const QRect &area)
{
  this->update(area);
}


