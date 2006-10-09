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

#include "vtkTimeStamp.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

#include <QtDebug>

#include "pqChartCoordinate.h"
#include "pqChartValue.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartPlotInternal
{
public:
  vtkSmartPointer<vtkRectilinearGrid> Data;
  vtkTimeStamp LastUpdateTime;
  pqVTKLineChartPlot::XAxisModes XAxisMode;
  QString XAxisArray;
  QString YAxisArray;
};

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::pqVTKLineChartPlot(vtkRectilinearGrid* dataset, QObject* p)
  : pqLineChartPlot(p)
{
  this->Internal = new pqVTKLineChartPlotInternal;
  this->Internal->Data = vtkRectilinearGrid::SafeDownCast(dataset);

}

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::~pqVTKLineChartPlot()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setXAxisMode(int mode)
{
  switch (mode)
    {
  case INDEX:
    this->Internal->XAxisMode = INDEX;
    break;
  case DATA_ARRAY:
    this->Internal->XAxisMode = DATA_ARRAY;
    break;
  case ARC_LENGTH:
    this->Internal->XAxisMode = ARC_LENGTH;
    break;
  default:
    qDebug() << "Mode not supported: " << mode;
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setXArray(const QString& name)
{
  this->Internal->XAxisArray = name;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setYArray(const QString& name)
{
  this->Internal->YAxisArray = name;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::update()
{
  if (this->Internal->LastUpdateTime < this->Internal->Data->GetMTime())
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
  return 1;
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getTotalNumberOfPoints() const
{
  return static_cast<int>(this->Internal->Data->GetNumberOfPoints());
}

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::SeriesType 
pqVTKLineChartPlot::getSeriesType(int vtkNotUsed(series)) const
{
  return pqVTKLineChartPlot::Line;
}

//-----------------------------------------------------------------------------
int pqVTKLineChartPlot::getNumberOfPoints(int vtkNotUsed(series)) const
{
  return static_cast<int>(this->Internal->Data->GetNumberOfPoints());
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getPoint(
  int series, int index, pqChartCoordinate& coord) const
{
  if (series == 0 && index >=0 && index < this->getNumberOfPoints(series))
    {
    double pt[2];
    pt[0] = this->getXPoint(index);
    pt[1] = this->getYPoint(index);
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
  switch (this->Internal->XAxisMode)
    {
  case INDEX:
      {
      int dims[3];
      this->Internal->Data->GetDimensions(dims);
      min = (double)0;
      max = (double)(dims[0]-1);
      }
    break;

  case DATA_ARRAY:
  case ARC_LENGTH:
      {
      vtkPointData* pd = this->Internal->Data->GetPointData();
      QString array_name = this->getXArrayNameToUse();
      vtkDataArray* array = pd->GetArray(array_name.toAscii().data());
      if (array)
        {
        double range[2];
        array->GetRange(range);
        min = range[0];
        max = range[1];
        }
      else
        {
        qDebug() << "Failed to locate X array " << array_name;
        min = 0;
        max = 1;
        }
      }
    break;
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  vtkPointData* pd = this->Internal->Data->GetPointData();
  vtkDataArray* array = pd->GetArray(
    this->Internal->YAxisArray.toAscii().data());
  if (array)
    {
    double range[2];
    array->GetRange(range);
    min = range[0];
    max = range[1];
    }
  else
    {
    qDebug() << "Failed to locate Y array " << this->Internal->YAxisArray;
    min = 0;
    max = 1;
    }
}

//-----------------------------------------------------------------------------
double pqVTKLineChartPlot::getXPoint(int index) const
{
  if (this->Internal->XAxisMode == INDEX)
    {
    return index;
    }
  vtkPointData* pd = this->Internal->Data->GetPointData();
  vtkDataArray* array = 
    pd->GetArray(this->getXArrayNameToUse().toAscii().data());
  if (array)
    {
    return array->GetTuple1(index);
    }
  return 0;
}

//-----------------------------------------------------------------------------
double pqVTKLineChartPlot::getYPoint(int index) const
{
  vtkPointData* pd = this->Internal->Data->GetPointData();
  vtkDataArray* array = pd->GetArray(
    this->Internal->YAxisArray.toAscii().data());
  if (array)
    {
    return array->GetTuple1(index);
    }
  return 0;
}

//-----------------------------------------------------------------------------
QString pqVTKLineChartPlot::getXArrayNameToUse() const
{
  if (this->Internal->XAxisMode == ARC_LENGTH)
    {
    return "arc_length";
    }
  return this->Internal->XAxisArray;
}

//-----------------------------------------------------------------------------
