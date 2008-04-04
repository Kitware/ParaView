/*=========================================================================

   Program: ParaView
   Module:    pqChartGridLayer.cxx

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

/// \file pqChartGridLayer.cxx
/// \date 2/8/2007

#include "pqChartGridLayer.h"

#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartAxisModel.h"
#include "pqChartAxisOptions.h"

#include <QPainter>
#include <QRect>


pqChartGridLayer::pqChartGridLayer(QObject *parentObject)
  : pqChartLayer(parentObject)
{
  this->Bounds = new QRect();
  this->LeftAxis = 0;
  this->TopAxis = 0;
  this->RightAxis = 0;
  this->BottomAxis = 0;
}

pqChartGridLayer::~pqChartGridLayer()
{
  delete this->Bounds;
}

void pqChartGridLayer::layoutChart(const QRect &area)
{
  // Set the bounding rectangle for the grid.
  *(this->Bounds) = area;
}

void pqChartGridLayer::drawChart(QPainter &painter, const QRect &area)
{
  // Make sure the area intersects the bounds.
  if(!this->Bounds->intersects(area))
    {
    return;
    }

  // Draw the axis grid lines.
  pqChartArea *chart = this->getChartArea();
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  this->drawAxisGrid(painter, chart->getAxis(pqChartAxis::Top));
  this->drawAxisGrid(painter, chart->getAxis(pqChartAxis::Right));
  this->drawAxisGrid(painter, chart->getAxis(pqChartAxis::Bottom));
  this->drawAxisGrid(painter, chart->getAxis(pqChartAxis::Left));
  painter.restore();
}

void pqChartGridLayer::setChartArea(pqChartArea *area)
{
  pqChartArea *oldArea = this->getChartArea();
  if(area == oldArea)
    {
    return;
    }

  if(oldArea)
    {
    // Disconnect from the axis signals.
    this->disconnect(
        oldArea->getAxis(pqChartAxis::Left)->getOptions(), 0, this, 0);
    this->disconnect(
        oldArea->getAxis(pqChartAxis::Bottom)->getOptions(), 0, this, 0);
    this->disconnect(
        oldArea->getAxis(pqChartAxis::Right)->getOptions(), 0, this, 0);
    this->disconnect(
        oldArea->getAxis(pqChartAxis::Top)->getOptions(), 0, this, 0);
    }

  pqChartLayer::setChartArea(area);
  if(area)
    {
    // Listen for axis grid option changes.
    this->connect(area->getAxis(pqChartAxis::Left)->getOptions(),
        SIGNAL(gridChanged()), this, SIGNAL(repaintNeeded()));
    this->connect(area->getAxis(pqChartAxis::Bottom)->getOptions(),
        SIGNAL(gridChanged()), this, SIGNAL(repaintNeeded()));
    this->connect(area->getAxis(pqChartAxis::Right)->getOptions(),
        SIGNAL(gridChanged()), this, SIGNAL(repaintNeeded()));
    this->connect(area->getAxis(pqChartAxis::Top)->getOptions(),
        SIGNAL(gridChanged()), this, SIGNAL(repaintNeeded()));
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

  float pixel = 0;
  bool vertical = axis->getLocation() == pqChartAxis::Left ||
      axis->getLocation() == pqChartAxis::Right;
  painter.setPen(options->getGridColor());
  for(int i = 0; i < total; i++)
    {
    // Only draw grid lines for visible tick marks.
    if(!axis->isLabelTickVisible(i))
      {
      continue;
      }

    // Only draw the grid lines inside the bounds.
    pixel = axis->getLabelLocation(i);
    if(vertical)
      {
      if((int)pixel > this->Bounds->bottom())
        {
        continue;
        }
      else if((int)pixel < this->Bounds->top())
        {
        break;
        }

      painter.drawLine(QPointF(this->Bounds->left(), pixel),
          QPointF(this->Bounds->right(), pixel));
      }
    else
      {
      if((int)pixel < this->Bounds->left())
        {
        continue;
        }
      else if((int)pixel > this->Bounds->right())
        {
        break;
        }

      painter.drawLine(QPointF(pixel, this->Bounds->top()),
          QPointF(pixel, this->Bounds->bottom()));
      }
    }
}


