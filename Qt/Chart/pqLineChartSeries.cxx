/*=========================================================================

   Program: ParaView
   Module:    pqLineChartSeries.cxx

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

/// \file pqLineChartSeries.cxx
/// \date 9/7/2006

#include "pqLineChartSeries.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"


pqLineChartSeries::pqLineChartSeries(QObject *parentObject)
  : QObject(parentObject)
{
  this->Axes = pqLineChartSeries::BottomLeft;
}

void pqLineChartSeries::setChartAxes(pqLineChartSeries::ChartAxes axes)
{
  if(this->Axes != axes)
    {
    this->Axes = axes;
    emit this->chartAxesChanged();
    }
}

void pqLineChartSeries::resetSeries()
{
  emit this->seriesReset();
}

void pqLineChartSeries::beginInsertPoints(int sequence, int first, int last)
{
  emit this->aboutToInsertPoints(sequence, first, last);
}

void pqLineChartSeries::endInsertPoints(int sequence)
{
  emit this->pointsInserted(sequence);
}

void pqLineChartSeries::beginRemovePoints(int sequence, int first, int last)
{
  emit this->aboutToRemovePoints(sequence, first, last);
}

void pqLineChartSeries::endRemovePoints(int sequence)
{
  emit this->pointsRemoved(sequence);
}

void pqLineChartSeries::beginMultiSequenceChange()
{
  emit this->aboutToChangeMultipleSequences();
}

void pqLineChartSeries::endMultiSequenceChange()
{
  emit this->changedMultipleSequences();
}

/* TODO: Add tooltips to new model-view charts.
const double pqLinePlot::getDistance(const QPoint& coords) const
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
}

void pqLineErrorPlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->PlotScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->PlotScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = pqChartCoordinate(this->Implementation->WorldCoords[i].X, this->Implementation->WorldCoords[i].Y);
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}

void ImageLineErrorPlot::showChartTip(QHelpEvent& event) const
{
  if(this->Image.isNull())
    {
    pqLineErrorPlot::showChartTip(event);
    return;
    }
  
  pqImageTip::showTip(this->Image, event.globalPos());
}
*/


