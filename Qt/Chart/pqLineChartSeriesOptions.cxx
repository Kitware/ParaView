/*=========================================================================

   Program: ParaView
   Module:    pqLineChartSeriesOptions.cxx

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

/// \file pqLineChartSeriesOptions.cxx
/// \date 9/18/2006

#include "pqLineChartSeriesOptions.h"

#include "pqPointMarker.h"
#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QVector>


class pqLineChartSeriesOptionsItem
{
public:
  pqLineChartSeriesOptionsItem();
  pqLineChartSeriesOptionsItem(const pqLineChartSeriesOptionsItem &other);
  ~pqLineChartSeriesOptionsItem() {}

  QPen Pen;
  QBrush Brush;
  pqPointMarker *Marker;
};


class pqLineChartSeriesOptionsInternal
{
public:
  pqLineChartSeriesOptionsInternal();
  ~pqLineChartSeriesOptionsInternal() {}

  QVector<pqLineChartSeriesOptionsItem> List;
  bool UseSameOptions;
};


//----------------------------------------------------------------------------
pqLineChartSeriesOptionsItem::pqLineChartSeriesOptionsItem()
  : Pen(), Brush()
{
  this->Marker = 0;
}

pqLineChartSeriesOptionsItem::pqLineChartSeriesOptionsItem(
    const pqLineChartSeriesOptionsItem &other)
  : Pen(other.Pen), Brush(other.Brush)
{
  this->Marker = other.Marker;
}


//----------------------------------------------------------------------------
pqLineChartSeriesOptionsInternal::pqLineChartSeriesOptionsInternal()
  : List()
{
  this->UseSameOptions = true;
}


//----------------------------------------------------------------------------
pqLineChartSeriesOptions::pqLineChartSeriesOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqLineChartSeriesOptionsInternal();
}

pqLineChartSeriesOptions::pqLineChartSeriesOptions(
    const pqLineChartSeriesOptions &other)
  : QObject(0)
{
  this->Internal = new pqLineChartSeriesOptionsInternal();
  this->Internal->UseSameOptions = other.Internal->UseSameOptions;

  // Copy the sequence options.
  this->Internal->List.reserve(other.Internal->List.size());
  QVector<pqLineChartSeriesOptionsItem>::Iterator iter =
      other.Internal->List.begin();
  for( ; iter != other.Internal->List.end(); ++iter)
    {
    this->Internal->List.append(*iter);
    }
}

pqLineChartSeriesOptions::~pqLineChartSeriesOptions()
{
  delete this->Internal;
}

bool pqLineChartSeriesOptions::isSequenceDependent() const
{
  return !this->Internal->UseSameOptions;
}

void pqLineChartSeriesOptions::setSequenceDependent(bool dependent)
{
  if(this->Internal->UseSameOptions != dependent)
    {
    return;
    }

  this->Internal->UseSameOptions = !dependent;
  if(this->Internal->UseSameOptions && this->Internal->List.size() > 1)
    {
    // Clean up the extra sequence options.
    QVector<pqLineChartSeriesOptionsItem>::Iterator iter =
        this->Internal->List.begin();
    this->Internal->List.erase(++iter, this->Internal->List.end());
    }

  // Signal that the options have changed.
  emit this->optionsChanged();
}

void pqLineChartSeriesOptions::getPen(QPen &pen, int sequence) const
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence >= 0 && sequence < this->Internal->List.size())
    {
    pen = this->Internal->List[sequence].Pen;
    }
}

void pqLineChartSeriesOptions::setPen(const QPen &pen, int sequence)
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence < 0)
    {
    return;
    }

  if(sequence >= this->Internal->List.size())
    {
    this->Internal->List.resize(sequence + 1);
    }

  this->Internal->List[sequence].Pen = pen;
  emit this->optionsChanged();
}

void pqLineChartSeriesOptions::getBrush(QBrush &brush, int sequence) const
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence >= 0 && sequence < this->Internal->List.size())
    {
    brush = this->Internal->List[sequence].Brush;
    }
}

void pqLineChartSeriesOptions::setBrush(const QBrush &brush, int sequence)
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence < 0)
    {
    return;
    }

  if(sequence >= this->Internal->List.size())
    {
    this->Internal->List.resize(sequence + 1);
    }

  this->Internal->List[sequence].Brush = brush;
  emit this->optionsChanged();
}

pqPointMarker *pqLineChartSeriesOptions::getMarker(int sequence) const
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence >= 0 && sequence < this->Internal->List.size())
    {
    return this->Internal->List[sequence].Marker;
    }

  return 0;
}

void pqLineChartSeriesOptions::setMarker(pqPointMarker *marker, int sequence)
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence < 0)
    {
    return;
    }

  if(sequence >= this->Internal->List.size())
    {
    this->Internal->List.resize(sequence + 1);
    }

  this->Internal->List[sequence].Marker = marker;
  emit this->optionsChanged();
}

void pqLineChartSeriesOptions::setupPainter(QPainter &painter,
    int sequence) const
{
  if(this->Internal->UseSameOptions)
    {
    sequence = 0;
    }

  if(sequence >= 0 && sequence < this->Internal->List.size())
    {
    painter.setPen(this->Internal->List[sequence].Pen);
    painter.setBrush(this->Internal->List[sequence].Brush);
    }
}

pqLineChartSeriesOptions &pqLineChartSeriesOptions::operator=(
    const pqLineChartSeriesOptions &other)
{
  this->Internal->UseSameOptions = other.Internal->UseSameOptions;
  this->Internal->List.clear();

  // Copy the sequence options.
  this->Internal->List.reserve(other.Internal->List.size());
  QVector<pqLineChartSeriesOptionsItem>::Iterator iter =
      other.Internal->List.begin();
  for( ; iter != other.Internal->List.end(); ++iter)
    {
    this->Internal->List.append(*iter);
    }

  return *this;
}


