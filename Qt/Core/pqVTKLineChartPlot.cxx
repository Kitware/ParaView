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
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"

#include <QColor>
#include <QtDebug>

#include "pqChartCoordinate.h"
#include "pqChartValue.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartPlotInternal
{
public:
  vtkTimeStamp LastUpdateTime;
  vtkSmartPointer<vtkDataArray> XAxisArray;
  vtkSmartPointer<vtkDataArray> YAxisArray;
  QColor Color;
};

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::pqVTKLineChartPlot(QObject* p)
  : pqLineChartPlot(p)
{
  this->Internal = new pqVTKLineChartPlotInternal;
}

//-----------------------------------------------------------------------------
pqVTKLineChartPlot::~pqVTKLineChartPlot()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setXArray(vtkDataArray* array)
{
  this->Internal->XAxisArray = array;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setYArray(vtkDataArray* array)
{
  this->Internal->YAxisArray = array;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::update()
{
  // TODO: Do the MTime stuff.
  this->forceUpdate();
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
  if (this->Internal->XAxisArray.GetPointer())
    {
    return static_cast<int>(this->Internal->XAxisArray->GetNumberOfTuples());
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
int pqVTKLineChartPlot::getNumberOfPoints(int vtkNotUsed(series)) const
{
  return this->getTotalNumberOfPoints();
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
  vtkDataArray* array = this->Internal->XAxisArray.GetPointer();
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
  vtkDataArray* array = this->Internal->YAxisArray.GetPointer();
  if (array)
    {
    double range[2];
    array->GetRange(range);
    min = range[0];
    max = range[1];
    }
  else
    {
    qDebug() << "Failed to locate Y array ";
    min = 0;
    max = 1;
    }
}

//-----------------------------------------------------------------------------
double pqVTKLineChartPlot::getXPoint(int index) const
{
  vtkDataArray* array = this->Internal->XAxisArray.GetPointer();
  if (array)
    {
    return array->GetTuple1(index);
    }
  return 0;
}

//-----------------------------------------------------------------------------
double pqVTKLineChartPlot::getYPoint(int index) const
{
  vtkDataArray* array = this->Internal->YAxisArray.GetPointer();
  if (array)
    {
    return array->GetTuple1(index);
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartPlot::setColor(const QColor& c) 
{
  this->Internal->Color = c;
}

//-----------------------------------------------------------------------------
QColor pqVTKLineChartPlot::getColor(int) const
{
  return this->Internal->Color;
}
