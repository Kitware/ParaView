/*=========================================================================

   Program: ParaView
   Module:    pqChartGridLayer.cxx

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

/// \file pqChartGridLayer.cxx
/// \date 2/8/2007

#include "pqChartGridLayer.h"

#include "pqChartAxisModel.h"
#include "pqChartAxisOptions.h"
#include "pqChartAxis.h"

#include <QPainter>
#include <QRect>


class pqChartGridLayerInternal
{
public:
  pqChartGridLayerInternal();
  ~pqChartGridLayerInternal() {}

  QRect Bounds;
};


//----------------------------------------------------------------------------
pqChartGridLayerInternal::pqChartGridLayerInternal()
  : Bounds()
{
}


//----------------------------------------------------------------------------
pqChartGridLayer::pqChartGridLayer(QObject *parentObject)
  : pqChartLayer(parentObject)
{
  this->Internal = new pqChartGridLayerInternal();
  this->LeftAxis = 0;
  this->TopAxis = 0;
  this->RightAxis = 0;
  this->BottomAxis = 0;
}

pqChartGridLayer::~pqChartGridLayer()
{
  delete this->Internal;
}

void pqChartGridLayer::layoutChart(const QRect &area)
{
  // Set the bounding rectangle for the grid.
  this->Internal->Bounds = area;
}

void pqChartGridLayer::drawChart(QPainter &painter, const QRect &area)
{
  // Make sure the area intersects the bounds.
  if(!this->Internal->Bounds.intersects(area))
    {
    return;
    }

  // Draw the axis grid lines.
  this->drawAxisGrid(painter, this->TopAxis);
  this->drawAxisGrid(painter, this->RightAxis);
  this->drawAxisGrid(painter, this->BottomAxis);
  this->drawAxisGrid(painter, this->LeftAxis);

  // Draw a border around the chart area.
  painter.setPen(Qt::darkGray);
  painter.drawRect(this->Internal->Bounds.x(), this->Internal->Bounds.y(),
      this->Internal->Bounds.width() - 1, this->Internal->Bounds.height() - 1);
}

void pqChartGridLayer::setLeftAxis(const pqChartAxis *axis)
{
  if(this->LeftAxis != axis)
    {
    if(this->LeftAxis)
      {
      this->disconnect(this->LeftAxis, 0, this, 0);
      }

    this->LeftAxis = axis;
    if(this->LeftAxis)
      {
      this->connect(this->LeftAxis->getOptions(), SIGNAL(gridChanged()),
          this, SIGNAL(repaintNeeded()));
      }
    }
}

void pqChartGridLayer::setTopAxis(const pqChartAxis *axis)
{
  if(this->TopAxis != axis)
    {
    if(this->TopAxis)
      {
      this->disconnect(this->TopAxis, 0, this, 0);
      }

    this->TopAxis = axis;
    if(this->TopAxis)
      {
      this->connect(this->TopAxis->getOptions(), SIGNAL(gridChanged()),
          this, SIGNAL(repaintNeeded()));
      }
    }
}

void pqChartGridLayer::setRightAxis(const pqChartAxis *axis)
{
  if(this->RightAxis != axis)
    {
    if(this->RightAxis)
      {
      this->disconnect(this->RightAxis, 0, this, 0);
      }

    this->RightAxis = axis;
    if(this->RightAxis)
      {
      this->connect(this->RightAxis->getOptions(), SIGNAL(gridChanged()),
          this, SIGNAL(repaintNeeded()));
      }
    }
}

void pqChartGridLayer::setBottomAxis(const pqChartAxis *axis)
{
  if(this->BottomAxis != axis)
    {
    if(this->BottomAxis)
      {
      this->disconnect(this->BottomAxis, 0, this, 0);
      }

    this->BottomAxis = axis;
    if(this->BottomAxis)
      {
      this->connect(this->BottomAxis->getOptions(), SIGNAL(gridChanged()),
          this, SIGNAL(repaintNeeded()));
      }
    }
}

void pqChartGridLayer::drawAxisGrid(QPainter &painter, const pqChartAxis *axis)
{
  if(!axis)
    {
    return;
    }

  // Use the axis options to get the color and visibility.
  pqChartAxisOptions *options = axis->getOptions();
  if(!options->isGridVisible())
    {
    return;
    }

  int total = 0;
  if(axis->getModel())
    {
    total = axis->getModel()->getNumberOfLabels();
    }

  int pixel = 0;
  bool vertical = axis->getLocation() == pqChartAxis::Left ||
      axis->getLocation() == pqChartAxis::Right;
  painter.setPen(options->getGridColor());
  for(int i = 0; i < total; i++)
    {
    // Only draw the grid lines inside the bounds.
    pixel = axis->getLabelLocation(i);
    if(vertical)
      {
      if(pixel > this->Internal->Bounds.bottom())
        {
        continue;
        }
      else if(pixel < this->Internal->Bounds.top())
        {
        break;
        }

      painter.drawLine(this->Internal->Bounds.left(), pixel,
          this->Internal->Bounds.right(), pixel);
      }
    else
      {
      if(pixel < this->Internal->Bounds.left())
        {
        continue;
        }
      else if(pixel > this->Internal->Bounds.right())
        {
        break;
        }

      painter.drawLine(pixel, this->Internal->Bounds.top(),
          pixel, this->Internal->Bounds.bottom());
      }
    }
}


