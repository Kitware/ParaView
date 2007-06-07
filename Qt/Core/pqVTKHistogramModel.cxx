/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramModel.cxx

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
#include "pqVTKHistogramModel.h"

#include "pqChartCoordinate.h"
#include <QtDebug>
#include "vtkDataArray.h"
#include "vtkSmartPointer.h"


class pqVTKHistogramModelInternal
{
public:
  pqVTKHistogramModelInternal();
  ~pqVTKHistogramModelInternal() {}

  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
  vtkSmartPointer<vtkDataArray> XArray;
  vtkSmartPointer<vtkDataArray> YArray;
};

//----------------------------------------------------------------------------
pqVTKHistogramModelInternal::pqVTKHistogramModelInternal()
  : Minimum(), Maximum()
{
  this->XArray = 0;
  this->YArray = 0;
}


//----------------------------------------------------------------------------
pqVTKHistogramModel::pqVTKHistogramModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqVTKHistogramModelInternal();
}

//----------------------------------------------------------------------------
pqVTKHistogramModel::~pqVTKHistogramModel()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
int pqVTKHistogramModel::getNumberOfBins() const
{
  if(this->Internal->YArray &&
      this->Internal->YArray->GetNumberOfComponents() == 1)
    {
    return this->Internal->YArray->GetNumberOfTuples();
    }

  return 0;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  if(index >= 0 && index < this->getNumberOfBins())
    {
    bin = this->Internal->YArray->GetTuple1(index);
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getBinRange(int index, pqChartValue &min,
    pqChartValue &max) const
{
  if(this->Internal->XArray && index >= 0 &&
      index + 1 < this->Internal->XArray->GetNumberOfTuples())
    {
    min = this->Internal->XArray->GetTuple1(index);
    max = this->Internal->XArray->GetTuple1(index + 1);
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::setDataArrays(vtkDataArray *xarray,
    vtkDataArray *yarray)
{
  if(xarray && yarray)
    {
    this->Internal->XArray = xarray;
    this->Internal->YArray = yarray;
    if(this->Internal->XArray->GetNumberOfTuples() < 2)
      {
      qWarning("The histogram range must have at least two values.");
      }

    // Get the overall range for the histogram. The bin ranges are
    // stored in the x coordinate array.
    double range[2];
    this->Internal->XArray->GetRange(range);
    this->Internal->Minimum.X = range[0];
    this->Internal->Maximum.X = range[1];

    this->Internal->YArray->GetRange(range);
    this->Internal->Minimum.Y = range[0];
    this->Internal->Maximum.Y = range[1];
    }
  else
    {
    this->Internal->XArray = 0;
    this->Internal->YArray = 0;

    // No data, just show empty chart.
    this->Internal->Minimum.Y = 0;
    this->Internal->Maximum.Y = 0;
    this->Internal->Minimum.X = 0;
    this->Internal->Maximum.X = 0;
    }

  emit this->histogramReset();
}


