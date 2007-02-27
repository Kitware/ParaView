/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartPlot.cxx

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
#include "pqVTKLineChartPlot.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkTimeStamp.h"

#include <QColor>
#include <QtDebug>
#include <QPointer>

#include "pqChartCoordinate.h"
#include "pqChartValue.h"
#include "pqLineChartDisplay.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartPlotInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqLineChartDisplay> Display;

  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
};

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::pqVTKLineChartPlot(pqLineChartDisplay* display, QObject* p)
  : pqLineChartPlot(p)
{
  this->Internal = new pqVTKLineChartPlotInternal;
  this->Internal->Display = display;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  // If any property changes, just to be on the safe side, 
  // we force the plot to be redrawn.
  this->Internal->VTKConnect->Connect(display->getProxy(), 
    vtkCommand::PropertyModifiedEvent, this, SLOT(markModified()));
  this->Internal->ModifiedTime.Modified();
}

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::~pqVTKLineChartPlot()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::markModified()
{
  this->Internal->ModifiedTime.Modified();
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::update()
{
  bool force_update = false;
    force_update = force_update || 
      (this->Internal->LastUpdateTime <= this->Internal->ModifiedTime);
  force_update = force_update || 
    (this->Internal->Display->getClientSideData() &&
     this->Internal->Display->getClientSideData()->GetMTime() > 
     this->Internal->LastUpdateTime);

  if (force_update)
    {
    this->forceUpdate();
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::forceUpdate()
{
  this->resetPlot();
  this->Internal->LastUpdateTime.Modified();
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getNumberOfSeries() const
{
  if (!this->Internal->Display->isVisible())
    {
    return 0;
    }

  int count = 0;
  for (int cc=0; cc < this->Internal->Display->getNumberOfYArrays(); cc++)
    {
    count += 
      (this->Internal->Display->getYArrayEnabled(cc) &&
       this->Internal->Display->getYArray(cc))? 1 : 0;
    }

  return count;
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getTotalNumberOfPoints() const
{
  vtkDataArray* xarray = this->Internal->Display->getXArray();
  if (xarray)
    {
    return static_cast<int>(xarray->GetNumberOfTuples());
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::SeriesType 
pqVTKLineChartPlot::getSeriesType(int vtkNotUsed(series)) const
{
  return pqVTKLineChartPlot::Line;
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getIndexFromSeries(int series) const
{
  int count = 0;
  for (int cc=0; cc < this->Internal->Display->getNumberOfYArrays(); cc++)
    {
    if  (this->Internal->Display->getYArrayEnabled(cc) &&
       this->Internal->Display->getYArray(cc))
      {
      if (count == series)
        {
        return cc;
        }
      count ++;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getNumberOfPoints(int series) const
{
  int index = this->getIndexFromSeries(series);
  if (index < 0)
    {
    return 0;
    }

  return static_cast<int>(
    this->Internal->Display->getYArray(index)->GetNumberOfTuples());
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getPoint(
  int series, int index, pqChartCoordinate& coord) const
{
  int array_index = this->getIndexFromSeries(series);

  if (array_index >= 0 && index >=0 && index < this->getNumberOfPoints(series))
    {
    double pt[2];
    pt[0] = this->Internal->Display->getXArray()->GetTuple1(index);;
    pt[1] = this->Internal->Display->getYArray(array_index)->GetTuple1(index);
    coord = pqChartCoordinate(pt[0], pt[1]);
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getErrorBounds(
  int vtkNotUsed(series), 
  int vtkNotUsed(index), 
  pqChartValue& vtkNotUsed(upper), 
  pqChartValue& vtkNotUsed(lower)) const
{
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getErrorWidth(
  int vtkNotUsed(series), pqChartValue& vtkNotUsed(width)) const
{
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  vtkDataArray* array = this->Internal->Display->getXArray();
  if (array)
    {
    double range[2];
    array->GetRange(range);
    min = range[0];
    max = range[1];
    }
  else
    {
    qDebug() << "Failed to locate X array";
    min = 0;
    max = 1;
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  double full_range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};

  for (int cc=0; cc < this->Internal->Display->getNumberOfYArrays(); cc++)
    {
    if (!this->Internal->Display->getYArrayEnabled(cc))
      {
      continue;
      }

    vtkDataArray* array = this->Internal->Display->getYArray(cc);
    if (!array)
      {
      continue;
      }

    double range[2];
    array->GetRange(range);
    full_range[0] = (full_range[0] > range[0])? range[0] : full_range[0];
    full_range[1] = (full_range[1] < range[1])? range[1] : full_range[1];
    }
  if (full_range[0] != VTK_DOUBLE_MAX && full_range[1] != VTK_DOUBLE_MIN)
    {
    min = full_range[0];
    max = full_range[1];
    }
  else
    {
    qDebug() << "Failed to locate any visible Y arrays ";
    min = 0;
    max = 1;
    }
}

//-----------------------------------------------------------------------------
QColor pqVTKLineChartPlot::getColor(int series) const
{
  int index  = this->getIndexFromSeries(series);
  if (index < 0)
    {
    return QColor(255,0,0);
    }
  return this->Internal->Display->getYColor(index);
}
