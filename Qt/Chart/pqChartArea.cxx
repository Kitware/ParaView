/*=========================================================================

   Program: ParaView
   Module:    pqChartArea.cxx

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

#include <QCoreApplication>
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
  pqChartAreaInternal();
  ~pqChartAreaInternal() {}

  QList<pqChartLayer *> Layers;  ///< Stores the chart layers.
  pqChartAreaAxisItem Option[4]; ///< Stores the axis behaviors.
  pqChartAxis *Axis[4];          ///< Stores the axis objects.
  int LocationIndex[4];          ///< Maps the axis location to an index.
  bool RangeChanged;             ///< True if the range has changed.
  bool InResize;                 ///< True if the widget is resizing.
  bool InZoom;                   ///< True if handling a zoom layout.
  bool SkipContextMenu;          ///< Used for context menu interaction.
  bool DelayContextMenu;         ///< Used for context menu interaction.
  bool ContextMenuBlocked;       ///< Used for context menu interaction.
  bool LayoutPending;            ///< Used to delay chart layout.
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
  this->RangeChanged = false;
  this->InResize = false;
  this->InZoom = false;
  this->SkipContextMenu = false;
  this->DelayContextMenu = false;
  this->ContextMenuBlocked = false;
  this->LayoutPending = false;

  // Set up the location to index map.
  this->LocationIndex[pqChartAxis::Left] = 0;
  this->LocationIndex[pqChartAxis::Bottom] = 1;
  this->LocationIndex[pqChartAxis::Right] = 2;
  this->LocationIndex[pqChartAxis::Top] = 3;
  
  // Initialize the axis pointers.
  for(int i = 0; i < 4; i++)
    {
    this->Axis[i] = 0;
    }
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

  // Set up the chart axes.
  this->setupAxes();

  // Add the grid layer and axis layer to the list.
  this->addLayer(this->GridLayer);
  this->addLayer(this->AxisLayer);
  this->Internal->RangeChanged = false;

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

  // Link the layout needed signal to the delay mechanism.
  this->connect(this, SIGNAL(delayedLayoutNeeded()),
      this, SLOT(layoutChart()), Qt::QueuedConnection);
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

pqChartAxis *pqChartArea::getAxis(pqChartAxis::AxisLocation location) const
{
  int index = this->Internal->LocationIndex[location];
  return this->Internal->Axis[index];
}

pqChartArea::AxisBehavior pqChartArea::getAxisBehavior(
    pqChartAxis::AxisLocation location) const
{
  int index = this->Internal->LocationIndex[location];
  return this->Internal->Option[index].Behavior;
}

void pqChartArea::setAxisBehavior(pqChartAxis::AxisLocation location,
    pqChartArea::AxisBehavior behavior)
{
  int index = this->Internal->LocationIndex[location];
  if(this->Internal->Option[index].Behavior != behavior)
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

  // Set the layer's reference to the chart area.
  chart->setChartArea(this);

  // Listen for the chart update signals.
  this->connect(chart, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
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
  chart->setChartArea(0);
  this->disconnect(chart, 0, this, 0);
  this->Internal->RangeChanged = true;
}

pqChartLayer *pqChartArea::getLayer(int index) const
{
  if(index >= 0 && index < this->Internal->Layers.size())
    {
    return this->Internal->Layers[index];
    }

  return 0;
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
  if(!(this->Internal->InResize || this->Internal->InZoom))
    {
    this->Internal->LayoutPending = false;
    }

  // If the axis layout behavior is ChartSelect, the axis could be fit
  // to the data range or determined by a chart. If the axis is data
  // driven, give the chart a chance to generate the axis labels.
  int i = 0;
  QList<pqChartLayer *>::Iterator layer;
  for( ; i < 4; i++)
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
  int left = this->Internal->LocationIndex[pqChartAxis::Left];
  int bottom = this->Internal->LocationIndex[pqChartAxis::Bottom];
  int right = this->Internal->LocationIndex[pqChartAxis::Right];
  int top = this->Internal->LocationIndex[pqChartAxis::Top];

  // Make sure there is enough vertical space. The top and bottom axes
  // know their preferred size before layout.
  QRect bounds = this->rect();
  int available = 0;
  int fontHeight = 0;
  if(this->Internal->Axis[left])
    {
    fontHeight = this->Internal->Axis[left]->getFontHeight();
    }

  if(this->Internal->Axis[right] &&
      this->Internal->Axis[right]->getFontHeight() > fontHeight)
    {
    fontHeight = this->Internal->Axis[right]->getFontHeight();
    }

  fontHeight /= 2;
  int space = 0;
  available = fontHeight;
  if(this->Internal->Axis[top])
    {
    space = this->Internal->Axis[top]->getPreferredSpace();
    if(space > fontHeight)
      {
      available = space;
      }
    }

  if(this->Internal->Axis[bottom])
    {
    space = this->Internal->Axis[bottom]->getPreferredSpace();
    available += space > fontHeight ? space : fontHeight;
    }
  else
    {
    available += fontHeight;
    }

  // Set the 'too small' flag on each of the axis objects.
  bool tooSmall = bounds.height() - available < TOO_SMALL_HEIGHT;
  for(i = 0; i < 4; i++)
    {
    if(this->Internal->Axis[i])
      {
      this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
      }
    }

  // Layout the left and right axes first.
  if(this->Internal->Axis[left])
    {
    this->Internal->Axis[left]->layoutAxis(bounds);
    }

  if(this->Internal->Axis[right])
    {
    this->Internal->Axis[right]->layoutAxis(bounds);
    }

  QRect axisBounds;
  if(!tooSmall)
    {
    // Make sure there is enough horizontal space.
    available = bounds.width();
    if(this->Internal->Axis[left])
      {
      this->Internal->Axis[left]->getBounds(axisBounds);
      available -= axisBounds.width();
      }

    if(this->Internal->Axis[right])
      {
      this->Internal->Axis[right]->getBounds(axisBounds);
      available -= axisBounds.width();
      }

    tooSmall = available < TOO_SMALL_WIDTH;
    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout the left and right axes.
      for(i = 0; i < 4; i++)
        {
        if(this->Internal->Axis[i])
          {
          this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
          }
        }

      if(this->Internal->Axis[left])
        {
        this->Internal->Axis[left]->layoutAxis(bounds);
        }

      if(this->Internal->Axis[right])
        {
        this->Internal->Axis[right]->layoutAxis(bounds);
        }
      }
    }

  // Layout the top and bottom axes. They need size from the left and
  // right axes layout.
  if(this->Internal->Axis[top])
    {
    this->Internal->Axis[top]->layoutAxis(bounds);
    }

  if(this->Internal->Axis[bottom])
    {
    this->Internal->Axis[bottom]->layoutAxis(bounds);
    if(this->Internal->Axis[top])
      {
      // The top and bottom axes should have the same width. The top
      // axis may need to be layed out again to account for the width
      // of the bottom axis labels.
      this->Internal->Axis[bottom]->getBounds(axisBounds);
      int bottomWidth = axisBounds.width();
      this->Internal->Axis[top]->getBounds(axisBounds);
      if(bottomWidth != axisBounds.width())
        {
        this->Internal->Axis[top]->layoutAxis(bounds);
        }
      }
    }

  if(!tooSmall)
    {
    // Check the horizontal space using the bounds from the top and
    // bottom axes.
    if(this->Internal->Axis[top])
      {
      this->Internal->Axis[top]->getBounds(axisBounds);
      tooSmall = axisBounds.width() < TOO_SMALL_WIDTH;
      }
    else if(this->Internal->Axis[bottom])
      {
      this->Internal->Axis[bottom]->getBounds(axisBounds);
      tooSmall = axisBounds.width() < TOO_SMALL_WIDTH;
      }

    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout all of the axes.
      for(i = 0; i < 4; i++)
        {
        if(this->Internal->Axis[i])
          {
          this->Internal->Axis[i]->setSpaceTooSmall(tooSmall);
          }
        }

      if(this->Internal->Axis[left])
        {
        this->Internal->Axis[left]->layoutAxis(bounds);
        }

      if(this->Internal->Axis[right])
        {
        this->Internal->Axis[right]->layoutAxis(bounds);
        }

      if(this->Internal->Axis[top])
        {
        this->Internal->Axis[top]->layoutAxis(bounds);
        }

      if(this->Internal->Axis[bottom])
        {
        this->Internal->Axis[bottom]->layoutAxis(bounds);
        }
      }
    else
      {
      // Adjust the size of the left and right axes. The top and bottom
      // axes may have needed more space.
      if(this->Internal->Axis[left])
        {
        this->Internal->Axis[left]->adjustAxisLayout();
        }

      if(this->Internal->Axis[right])
        {
        this->Internal->Axis[right]->adjustAxisLayout();
        }
      }
    }

  // Calculate the area inside the axes.
  QRect chartBounds = bounds;
  if(this->Internal->Axis[left])
    {
    this->Internal->Axis[left]->getBounds(bounds);
    chartBounds.setLeft(bounds.right());
    chartBounds.setTop(bounds.top());
    chartBounds.setBottom(bounds.bottom());
    }

  if(this->Internal->Axis[top])
    {
    this->Internal->Axis[top]->getBounds(bounds);
    chartBounds.setLeft(bounds.left());
    chartBounds.setTop(bounds.bottom());
    chartBounds.setRight(bounds.right());
    }

  if(this->Internal->Axis[right])
    {
    this->Internal->Axis[right]->getBounds(bounds);
    chartBounds.setTop(bounds.top());
    chartBounds.setRight(bounds.left());
    chartBounds.setBottom(bounds.bottom());
    }

  if(this->Internal->Axis[bottom])
    {
    this->Internal->Axis[bottom]->getBounds(bounds);
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

void pqChartArea::updateLayout()
{
  if(!this->Internal->LayoutPending)
    {
    this->Internal->LayoutPending = true;
    emit this->delayedLayoutNeeded();
    }
}

bool pqChartArea::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    // Set the font for each of the axes. Ignore the layout request
    // signals so all the axes can be updated before relayout.
    for(int i = 0; i < 4; i++)
      {
      if(this->Internal->Axis[i])
        {
        this->Internal->Axis[i]->blockSignals(true);
        this->Internal->Axis[i]->getOptions()->setLabelFont(this->font());
        this->Internal->Axis[i]->blockSignals(false);
        }
      }

    // Layout the chart area.
    this->updateLayout();
    }
  else if(e->type() == QEvent::ContextMenu)
    {
    QContextMenuEvent *cme = static_cast<QContextMenuEvent *>(e);
    if(cme->reason() == QContextMenuEvent::Mouse &&
        (this->Internal->SkipContextMenu || this->Internal->DelayContextMenu))
      {
      this->Internal->SkipContextMenu = false;
      if(this->Internal->DelayContextMenu)
        {
        this->Internal->ContextMenuBlocked = true;
        }

      e->accept();
      return true;
      }
    }
  // TODO
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

  // If the mouse button is the right button, delay the context menu.
  if(e->button() == Qt::RightButton)
    {
    this->Internal->DelayContextMenu = true;
    }

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
  // When the mouse is moved, the context menu should not pop-up.
  if(e->buttons() & Qt::RightButton)
    {
    this->Internal->SkipContextMenu = true;
    this->Internal->DelayContextMenu = false;
    }

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

  if(e->button() == Qt::RightButton)
    {
    if(this->Internal->ContextMenuBlocked)
      {
      if(this->Internal->SkipContextMenu)
        {
        this->Internal->SkipContextMenu = false;
        }
      else if(this->Internal->DelayContextMenu)
        {
        // Re-send the context menu event.
        QContextMenuEvent *cme = new QContextMenuEvent(
            QContextMenuEvent::Mouse, e->pos(), e->globalPos());
        QCoreApplication::postEvent(this, cme);
        }
      }

    this->Internal->ContextMenuBlocked = false;
    this->Internal->DelayContextMenu = false;
    }
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
    this->Internal->InZoom = true;
    this->layoutChart();
    this->Internal->InZoom = false;
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

void pqChartArea::setupAxes()
{
  // Create an axis object for each location.
  pqChartAxis::AxisLocation location = pqChartAxis::Left;
  int left = this->Internal->LocationIndex[location];
  this->Internal->Axis[left] = new pqChartAxis(location, this);
  this->Internal->Axis[left]->setObjectName("LeftAxis");
  pqChartAxisModel *model = new pqChartAxisModel(this);
  model->setObjectName("LeftAxisModel");
  this->Internal->Axis[left]->setModel(model);
  this->Internal->Axis[left]->setContentsSpace(this->Contents);

  location = pqChartAxis::Bottom;
  int bottom = this->Internal->LocationIndex[location];
  this->Internal->Axis[bottom] = new pqChartAxis(location, this);
  this->Internal->Axis[bottom]->setObjectName("BottomAxis");
  model = new pqChartAxisModel(this);
  model->setObjectName("BottomAxisModel");
  this->Internal->Axis[bottom]->setModel(model);
  this->Internal->Axis[bottom]->setContentsSpace(this->Contents);

  location = pqChartAxis::Right;
  int right = this->Internal->LocationIndex[location];
  this->Internal->Axis[right] = new pqChartAxis(location, this);
  this->Internal->Axis[right]->setObjectName("RightAxis");
  model = new pqChartAxisModel(this);
  model->setObjectName("RightAxisModel");
  this->Internal->Axis[right]->setModel(model);
  this->Internal->Axis[right]->setContentsSpace(this->Contents);

  location = pqChartAxis::Top;
  int top = this->Internal->LocationIndex[location];
  this->Internal->Axis[top] = new pqChartAxis(location, this);
  this->Internal->Axis[top]->setObjectName("TopAxis");
  model = new pqChartAxisModel(this);
  model->setObjectName("TopAxisModel");
  this->Internal->Axis[top]->setModel(model);
  this->Internal->Axis[top]->setContentsSpace(this->Contents);

  // Set up the axis neighbors and the parallel axis.
  this->Internal->Axis[left]->setNeigbors(
      this->Internal->Axis[bottom], this->Internal->Axis[top]);
  this->Internal->Axis[bottom]->setNeigbors(
      this->Internal->Axis[left], this->Internal->Axis[right]);
  this->Internal->Axis[right]->setNeigbors(
      this->Internal->Axis[bottom], this->Internal->Axis[top]);
  this->Internal->Axis[top]->setNeigbors(
      this->Internal->Axis[left], this->Internal->Axis[right]);

  this->Internal->Axis[left]->setParallelAxis(this->Internal->Axis[right]);
  this->Internal->Axis[bottom]->setParallelAxis(this->Internal->Axis[top]);
  this->Internal->Axis[right]->setParallelAxis(this->Internal->Axis[left]);
  this->Internal->Axis[top]->setParallelAxis(this->Internal->Axis[bottom]);

  // Listen to the axis update signals.
  for(int i = 0; i < 4; i++)
    {
    this->connect(this->Internal->Axis[i], SIGNAL(layoutNeeded()),
        this, SLOT(updateLayout()));
    this->connect(this->Internal->Axis[i], SIGNAL(repaintNeeded()),
        this, SLOT(update()));
    }
}


