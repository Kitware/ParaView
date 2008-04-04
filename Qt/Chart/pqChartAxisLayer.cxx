/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisLayer.cxx

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

/// \file pqChartAxisLayer.cxx
/// \date 2/9/2007

#include "pqChartAxisLayer.h"

#include "pqChartArea.h"
#include "pqChartAxis.h"

#include <QPainter>
#include <QRect>


pqChartAxisLayer::pqChartAxisLayer(QObject *parentObject)
  : pqChartLayer(parentObject)
{
  this->Bounds = new QRect();
}

pqChartAxisLayer::~pqChartAxisLayer()
{
  delete this->Bounds;
}

void pqChartAxisLayer::layoutChart(const QRect &area)
{
  *(this->Bounds) = area;
}

void pqChartAxisLayer::drawChart(QPainter &painter, const QRect &area)
{
  // Draw a border around the chart area.
  painter.setPen(Qt::darkGray);
  painter.drawRect(this->Bounds->x(), this->Bounds->y(),
      this->Bounds->width() - 1, this->Bounds->height() - 1);

  // Draw each of the axes.
  pqChartArea *chart = this->getChartArea();
  chart->getAxis(pqChartAxis::Top)->drawAxis(painter, area);
  chart->getAxis(pqChartAxis::Right)->drawAxis(painter, area);
  chart->getAxis(pqChartAxis::Bottom)->drawAxis(painter, area);
  chart->getAxis(pqChartAxis::Left)->drawAxis(painter, area);
}


