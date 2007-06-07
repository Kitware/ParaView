/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartSeries.cxx

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
#include "pqVTKLineChartSeries.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"


class pqVTKLineChartSeriesInternal
{
public:
  pqVTKLineChartSeriesInternal();
  ~pqVTKLineChartSeriesInternal() {}

  vtkSmartPointer<vtkDataArray> XArray;
  vtkSmartPointer<vtkDataArray> YArray;
};


//-----------------------------------------------------------------------------
pqVTKLineChartSeriesInternal::pqVTKLineChartSeriesInternal()
{
  this->XArray = 0;
  this->YArray = 0;
}


//-----------------------------------------------------------------------------
pqVTKLineChartSeries::pqVTKLineChartSeries(QObject *parentObject)
  : pqLineChartSeries(parentObject)
{
  this->Internal = new pqVTKLineChartSeriesInternal();
}

pqVTKLineChartSeries::~pqVTKLineChartSeries()
{
  delete this->Internal;
}

int pqVTKLineChartSeries::getNumberOfSequences() const
{
  if(this->Internal->XArray && this->Internal->YArray)
    {
    return 1;
    }

  return 0;
}

int pqVTKLineChartSeries::getTotalNumberOfPoints() const
{
  return this->getNumberOfPoints(0);
}

pqLineChartSeries::SequenceType pqVTKLineChartSeries::getSequenceType(int) const
{
  return pqLineChartSeries::Line;
}

int pqVTKLineChartSeries::getNumberOfPoints(int sequence) const
{
  if(this->Internal->YArray && sequence == 0)
    {
    return static_cast<int>(this->Internal->YArray->GetNumberOfTuples());
    }

  return 0;
}

void pqVTKLineChartSeries::getPoint(int sequence, int index,
    pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->getNumberOfPoints(sequence))
    {
    coord.X = this->Internal->XArray->GetTuple1(index);;
    coord.Y = this->Internal->YArray->GetTuple1(index);
    }
}

void pqVTKLineChartSeries::getErrorBounds(int, int, pqChartValue &,
    pqChartValue &) const
{
}

void pqVTKLineChartSeries::getErrorWidth(int , pqChartValue &) const
{
}

void pqVTKLineChartSeries::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  if(this->Internal->XArray)
    {
    double range[2];
    this->Internal->XArray->GetRange(range);
    min = range[0];
    max = range[1];
    }
  else
    {
    min = (double)0.0;
    max = (double)1.0;
    }
}

void pqVTKLineChartSeries::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  if(this->Internal->YArray)
    {
    double range[2];
    this->Internal->YArray->GetRange(range);
    min = range[0];
    max = range[1];
    }
  else
    {
    min = (double)0.0;
    max = (double)1.0;
    }
}

void pqVTKLineChartSeries::setDataArrays(vtkDataArray *xArray,
    vtkDataArray *yArray)
{
  if(xArray && yArray)
    {
    this->Internal->XArray = xArray;
    this->Internal->YArray = yArray;
    }
  else
    {
    this->Internal->XArray = 0;
    this->Internal->YArray = 0;
    }

  this->resetSeries();
}


