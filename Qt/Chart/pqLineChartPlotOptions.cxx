/*=========================================================================

   Program: ParaView
   Module:    pqLineChartPlotOptions.cxx

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

/// \file pqLineChartPlotOptions.cxx
/// \date 9/18/2006

#include "pqLineChartPlotOptions.h"

#include "pqPointMarker.h"
#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QVector>


class pqLineChartPlotOptionsItem
{
public:
  pqLineChartPlotOptionsItem();
  ~pqLineChartPlotOptionsItem() {}

  QPen Pen;
  QBrush Brush;
  pqPointMarker * Marker;
};


class pqLineChartPlotOptionsInternal
{
public:
  pqLineChartPlotOptionsInternal();
  ~pqLineChartPlotOptionsInternal() {}

  QVector<pqLineChartPlotOptionsItem> List;
};


//----------------------------------------------------------------------------
pqLineChartPlotOptionsItem::pqLineChartPlotOptionsItem()
  : Pen(), Brush()
{
  this->Marker = 0;
}


//----------------------------------------------------------------------------
pqLineChartPlotOptionsInternal::pqLineChartPlotOptionsInternal()
  : List()
{
}


//----------------------------------------------------------------------------
pqLineChartPlotOptions::pqLineChartPlotOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqLineChartPlotOptionsInternal();
}

pqLineChartPlotOptions::~pqLineChartPlotOptions()
{
  delete this->Internal;
}

void pqLineChartPlotOptions::setPen(int series, const QPen &pen)
{
  if(series < 0)
    {
    return;
    }

  if(series >= this->Internal->List.size())
    {
    this->Internal->List.resize(series + 1);
    }

  this->Internal->List[series].Pen = pen;
  emit this->optionsChanged();
}

void pqLineChartPlotOptions::setBrush(int series, const QBrush &brush)
{
  if(series < 0)
    {
    return;
    }

  if(series >= this->Internal->List.size())
    {
    this->Internal->List.resize(series + 1);
    }

  this->Internal->List[series].Brush = brush;
  emit this->optionsChanged();
}

void pqLineChartPlotOptions::setMarker(int series, pqPointMarker *marker)
{
  if(series < 0)
    {
    return;
    }

  if(series >= this->Internal->List.size())
    {
    this->Internal->List.resize(series + 1);
    }

  this->Internal->List[series].Marker = marker;
  emit this->optionsChanged();
}

void pqLineChartPlotOptions::setupPainter(QPainter &painter, int series) const
{
  if(series >= 0 && series < this->Internal->List.size())
    {
    painter.setPen(this->Internal->List[series].Pen);
    painter.setBrush(this->Internal->List[series].Brush);
    }
}

pqPointMarker *pqLineChartPlotOptions::getMarker(int series) const
{
  if(series >= 0 && series < this->Internal->List.size())
    {
    return this->Internal->List[series].Marker;
    }

  return 0;
}


