/*=========================================================================

   Program: ParaView
   Module:    pqSimpleLineChartSeries.cxx

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

/// \file pqSimpleLineChartSeries.cxx
/// \date 8/22/2005

#include "pqSimpleLineChartSeries.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"

#include <QHelpEvent>
#include <QList>
#include <QToolTip>
#include <QVector>

class pqSimpleLineChartSeriesErrorBounds
{
public:
  pqSimpleLineChartSeriesErrorBounds();
  ~pqSimpleLineChartSeriesErrorBounds() {}

  pqChartValue Upper;
  pqChartValue Lower;
};


class pqSimpleLineChartSeriesErrorData
{
public:
  pqSimpleLineChartSeriesErrorData();
  ~pqSimpleLineChartSeriesErrorData() {}

  QVector<pqSimpleLineChartSeriesErrorBounds> Bounds;
  pqChartValue Width;
};


class pqSimpleLineChartSeriesSequence
{
public:
  pqSimpleLineChartSeriesSequence(pqLineChartSeries::SequenceType type);
  ~pqSimpleLineChartSeriesSequence();

  QVector<pqChartCoordinate> Points;
  pqLineChartSeries::SequenceType Type;
  pqSimpleLineChartSeriesErrorData *Error;
};


class pqSimpleLineChartSeriesInternal
{
public:
  pqSimpleLineChartSeriesInternal();
  ~pqSimpleLineChartSeriesInternal() {}

  QList<pqSimpleLineChartSeriesSequence *> Sequences;
  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
};


//----------------------------------------------------------------------------
pqSimpleLineChartSeriesErrorBounds::pqSimpleLineChartSeriesErrorBounds()
  : Upper(), Lower()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartSeriesErrorData::pqSimpleLineChartSeriesErrorData()
  : Bounds(), Width()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartSeriesSequence::pqSimpleLineChartSeriesSequence(
    pqLineChartSeries::SequenceType type)
  : Points()
{
  this->Type = type;
  this->Error = 0;
  if(this->Type == pqLineChartSeries::Error)
    {
    this->Error = new pqSimpleLineChartSeriesErrorData();
    }
}

pqSimpleLineChartSeriesSequence::~pqSimpleLineChartSeriesSequence()
{
  if(this->Error)
    {
    delete this->Error;
    }
}


//----------------------------------------------------------------------------
pqSimpleLineChartSeriesInternal::pqSimpleLineChartSeriesInternal()
  : Sequences(), Minimum(), Maximum()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartSeries::pqSimpleLineChartSeries(QObject *parentObject)
  : pqLineChartSeries(parentObject)
{
  this->Internal = new pqSimpleLineChartSeriesInternal();
}

pqSimpleLineChartSeries::~pqSimpleLineChartSeries()
{
  QList<pqSimpleLineChartSeriesSequence *>::Iterator sequence =
      this->Internal->Sequences.begin();
  for( ; sequence != this->Internal->Sequences.end(); ++sequence)
    {
    delete *sequence;
    }

  delete this->Internal;
}

int pqSimpleLineChartSeries::getNumberOfSequences() const
{
  return this->Internal->Sequences.size();
}

int pqSimpleLineChartSeries::getTotalNumberOfPoints() const
{
  int total = 0;
  QList<pqSimpleLineChartSeriesSequence *>::Iterator sequence =
      this->Internal->Sequences.begin();
  for( ; sequence != this->Internal->Sequences.end(); ++sequence)
    {
    total += (*sequence)->Points.size();
    }

  return total;
}

pqLineChartSeries::SequenceType pqSimpleLineChartSeries::getSequenceType(
    int sequence) const
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences())
    {
    return this->Internal->Sequences[sequence]->Type;
    }

  return pqLineChartSeries::Invalid;
}

int pqSimpleLineChartSeries::getNumberOfPoints(int sequence) const
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences())
    {
    return this->Internal->Sequences[sequence]->Points.size();
    }

  return 0;
}

bool pqSimpleLineChartSeries::getPoint(int sequence, int index,
    pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->getNumberOfPoints(sequence))
    {
    coord = this->Internal->Sequences[sequence]->Points[index];
    }

  return false;
}

void pqSimpleLineChartSeries::getErrorBounds(int sequence, int index,
    pqChartValue &upper, pqChartValue &lower) const
{
  if(this->getSequenceType(sequence) == pqLineChartSeries::Error)
    {
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    if(array->Error && index >= 0 && index < array->Error->Bounds.size())
      {
      upper = array->Error->Bounds[index].Upper;
      lower = array->Error->Bounds[index].Lower;
      }
    }
}

void pqSimpleLineChartSeries::getErrorWidth(int sequence,
    pqChartValue &width) const
{
  if(this->getSequenceType(sequence) == pqLineChartSeries::Error)
    {
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    if(array->Error)
      {
      width = array->Error->Width;
      }
    }
}

void pqSimpleLineChartSeries::getRangeX(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqSimpleLineChartSeries::getRangeY(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqSimpleLineChartSeries::clearSeries()
{
  if(this->Internal->Sequences.size() > 0)
    {
    QList<pqSimpleLineChartSeriesSequence *>::Iterator iter =
        this->Internal->Sequences.begin();
    for( ; iter != this->Internal->Sequences.end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->Sequences.clear();
    this->updateSeriesRanges();
    this->resetSeries();
    }
}

void pqSimpleLineChartSeries::addSequence(pqLineChartSeries::SequenceType type)
{
  this->Internal->Sequences.append(new pqSimpleLineChartSeriesSequence(type));
  this->resetSeries();
}

void pqSimpleLineChartSeries::insertSequence(int index,
    pqLineChartSeries::SequenceType type)
{
  if(index >= 0 && index < this->getNumberOfSequences())
    {
    this->Internal->Sequences.insert(index,
        new pqSimpleLineChartSeriesSequence(type));
    this->resetSeries();
    }
}

void pqSimpleLineChartSeries::setSequenceType(int sequence,
    pqLineChartSeries::SequenceType type)
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences())
    {
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    if(array->Type != type)
      {
      if(array->Error)
        {
        delete array->Error;
        array->Error = 0;
        }

      array->Type = type;
      if(array->Type == pqLineChartSeries::Error)
        {
        array->Error = new pqSimpleLineChartSeriesErrorData();
        array->Error->Bounds.resize(array->Points.size());
        }

      this->resetSeries();
      }
    }
}

void pqSimpleLineChartSeries::removeSequence(int sequence)
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences())
    {
    delete this->Internal->Sequences.takeAt(sequence);
    this->updateSeriesRanges();
    this->resetSeries();
    }
}

void pqSimpleLineChartSeries::copySequencePoints(int source, int destination)
{
  if(source >= 0 && source < this->getNumberOfSequences() &&
      destination >= 0 && destination < this->getNumberOfSequences())
    {
    // If the destination already has points, notify the view that
    // they will be removed.
    this->clearPoints(destination);

    // Copy the source points to the destination. Notify the view
    // that points are being added to the destination.
    if(this->getNumberOfPoints(source) > 0)
      {
      int last = this->getNumberOfPoints(source) - 1;
      this->beginInsertPoints(destination, 0, last);
      pqSimpleLineChartSeriesSequence *destPlot =
          this->Internal->Sequences[destination];
      destPlot->Points = this->Internal->Sequences[source]->Points;
      if(destPlot->Error)
        {
        destPlot->Error->Bounds.resize(destPlot->Points.size());
        }

      this->endInsertPoints(destination);
      }
    }
}

void pqSimpleLineChartSeries::addPoint(int sequence,
    const pqChartCoordinate &coord)
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences())
    {
    int total = this->getNumberOfPoints(sequence);
    this->beginInsertPoints(sequence, total, total);
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    array->Points.append(coord);
    if(array->Error)
      {
      array->Error->Bounds.resize(array->Points.size());
      }

    this->updateSeriesRanges(coord);
    this->endInsertPoints(sequence);
    }
}

void pqSimpleLineChartSeries::insertPoint(int sequence, int index,
    const pqChartCoordinate &coord)
{
  if(index >= 0 && index < this->getNumberOfPoints(sequence))
    {
    this->beginInsertPoints(sequence, index, index);
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    array->Points.insert(index, coord);
    if(array->Error && index < array->Error->Bounds.size())
      {
      array->Error->Bounds.insert(index,
          pqSimpleLineChartSeriesErrorBounds());
      }

    this->updateSeriesRanges(coord);
    this->endInsertPoints(sequence);
    }
}

void pqSimpleLineChartSeries::removePoint(int sequence, int index)
{
  if(index >= 0 && index < this->getNumberOfPoints(sequence))
    {
    this->beginRemovePoints(sequence, index, index);
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    array->Points.remove(index);
    if(array->Error && index < array->Error->Bounds.size())
      {
      array->Error->Bounds.remove(index);
      }

    this->updateSeriesRanges();
    this->endRemovePoints(sequence);
    }
}

void pqSimpleLineChartSeries::clearPoints(int sequence)
{
  if(sequence >= 0 && sequence < this->getNumberOfSequences() &&
      this->Internal->Sequences[sequence]->Points.size() > 0)
    {
    // Notify the view that the points are being removed. Then, clear
    // the point sequence.
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    int last = array->Points.size() - 1;
    this->beginRemovePoints(sequence, 0, last);
    array->Points.clear();
    if(array->Error)
      {
      array->Error->Bounds.clear();
      }

    this->updateSeriesRanges();
    this->endRemovePoints(sequence);
    }
}

void pqSimpleLineChartSeries::setErrorBounds(int sequence, int index,
    const pqChartValue &upper, const pqChartValue &lower)
{
  if(this->getSequenceType(sequence) == pqLineChartSeries::Error)
    {
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    if(array->Error && index >= 0 && index < array->Error->Bounds.size())
      {
      array->Error->Bounds[index].Upper = upper;
      array->Error->Bounds[index].Lower = lower;

      // Adjust the y-axis range for the new error bounds.
      if(lower < this->Internal->Minimum.Y)
        {
        this->Internal->Minimum.Y = lower;
        }
      if(upper > this->Internal->Maximum.Y)
        {
        this->Internal->Maximum.Y = upper;
        }

      emit this->errorBoundsChanged(sequence, index, index);
      }
    }
}

void pqSimpleLineChartSeries::setErrorWidth(int sequence,
    const pqChartValue &width)
{
  if(this->getSequenceType(sequence) == pqLineChartSeries::Error)
    {
    pqSimpleLineChartSeriesSequence *array =
        this->Internal->Sequences[sequence];
    if(array->Error)
      {
      array->Error->Width = width;
      emit this->errorWidthChanged(sequence);
      }
    }
}

void pqSimpleLineChartSeries::updateSeriesRanges()
{
  this->Internal->Minimum.X = (int)0;
  this->Internal->Minimum.Y = (int)0;
  this->Internal->Maximum.X = (int)0;
  this->Internal->Maximum.Y = (int)0;
  bool firstSet = false;
  QList<pqSimpleLineChartSeriesSequence *>::Iterator sequence =
      this->Internal->Sequences.begin();
  for( ; sequence != this->Internal->Sequences.end(); ++sequence)
    {
    QVector<pqChartCoordinate>::Iterator point = (*sequence)->Points.begin();
    for( ; point != (*sequence)->Points.end(); ++point)
      {
      if(firstSet)
        {
        if(point->X < this->Internal->Minimum.X)
          {
          this->Internal->Minimum.X = point->X;
          }
        else if(point->X > this->Internal->Maximum.X)
          {
          this->Internal->Maximum.X = point->X;
          }

        if(point->Y < this->Internal->Minimum.Y)
          {
          this->Internal->Minimum.Y = point->Y;
          }
        else if(point->Y > this->Internal->Maximum.Y)
          {
          this->Internal->Maximum.Y = point->Y;
          }
        }
      else
        {
        firstSet = true;
        this->Internal->Minimum.X = point->X;
        this->Internal->Minimum.Y = point->Y;
        this->Internal->Maximum.X = point->X;
        this->Internal->Maximum.Y = point->Y;
        }
      }

    // Account for the error bounds in the y-axis range.
    if((*sequence)->Error)
      {
      QVector<pqSimpleLineChartSeriesErrorBounds>::Iterator bounds =
          (*sequence)->Error->Bounds.begin();
      for( ; bounds != (*sequence)->Error->Bounds.end(); ++bounds)
        {
        // If the error bounds are equal, they are ignored.
        if(bounds->Upper != bounds->Lower)
          {
          if(bounds->Lower < this->Internal->Minimum.Y)
            {
            this->Internal->Minimum.Y = bounds->Lower;
            }
          if(bounds->Upper > this->Internal->Maximum.Y)
            {
            this->Internal->Maximum.Y = bounds->Upper;
            }
          }
        }
      }
    }
}

void pqSimpleLineChartSeries::updateSeriesRanges(const pqChartCoordinate &coord)
{
  if(this->getTotalNumberOfPoints() == 1)
    {
    this->Internal->Minimum.X = coord.X;
    this->Internal->Minimum.Y = coord.Y;
    this->Internal->Maximum.X = coord.X;
    this->Internal->Maximum.Y = coord.Y;
    }
  else
    {
    if(coord.X < this->Internal->Minimum.X)
      {
      this->Internal->Minimum.X = coord.X;
      }
    else if(coord.X > this->Internal->Maximum.X)
      {
      this->Internal->Maximum.X = coord.X;
      }

    if(coord.Y < this->Internal->Minimum.Y)
      {
      this->Internal->Minimum.Y = coord.Y;
      }
    else if(coord.Y > this->Internal->Maximum.Y)
      {
      this->Internal->Maximum.Y = coord.Y;
      }
    }
}


/*const double pqLinePlot::getDistance(const QPoint& coords) const
{
  double distance = VTK_DOUBLE_MAX;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    distance = vtkstd::min(distance, static_cast<double>((this->Implementation->ScreenCoords[i] - coords).manhattanLength()));
  return distance;
}

void pqLinePlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->ScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = this->Implementation->WorldCoords[i];
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}*/
