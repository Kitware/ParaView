/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramModel.cxx

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
#include "pqVTKHistogramModel.h"

#include "pqChartCoordinate.h"
#include <QtDebug>
#include "vtkDataArray.h"
#include "vtkSmartPointer.h"


class pqVTKHistogramModelInternal
{
public:
  pqVTKHistogramModelInternal():
    Minimum(), Maximum(), XArrayComponent(-1), YArrayComponent(-1), XDelta(0) {}

  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
  vtkSmartPointer<vtkDataArray> XArray;
  vtkSmartPointer<vtkDataArray> YArray;
  int XArrayComponent;
  int YArrayComponent;
  double XDelta;
};

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
  if (this->Internal->XArray)
    {
    return this->Internal->XArray->GetNumberOfTuples();
    }

  return 0;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  if (index >= 0 && index < this->getNumberOfBins())
    {
    bin = this->Internal->YArray->GetComponent(index,
      this->Internal->YArrayComponent);
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getBinRange(int index, pqChartValue &min,
    pqChartValue &max) const
{
  if (this->Internal->XArray &&
    index >= 0 && index < this->getNumberOfBins())
    {
    double bin_mid_point = this->Internal->XArray->GetComponent(index,
      this->Internal->XArrayComponent);
    min = bin_mid_point - this->Internal->XDelta; 
    max = bin_mid_point + this->Internal->XDelta; 
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

#define PQ_MAX(x, y) ((x)>(y))?(x):(y)

//----------------------------------------------------------------------------
void pqVTKHistogramModel::setDataArrays(vtkDataArray *xarray, int xcomp,
  vtkDataArray *yarray, int ycomp)
{
  if(xarray && yarray)
    {
    this->Internal->XArray = xarray;
    this->Internal->YArray = yarray;
    if (xcomp >= xarray->GetNumberOfComponents())
      {
      qWarning() << "Requested x-component " << xcomp << " is invalid.";
      xcomp = -1;
      }
    if (ycomp >= yarray->GetNumberOfComponents())
      {
      qWarning() << "Requested y-component " << ycomp << " is invalid.";
      ycomp = -1;
      }

    this->Internal->XArrayComponent = xcomp;
    this->Internal->YArrayComponent = ycomp;

    // Get the overall range for the histogram. The bin ranges are
    // stored in the x coordinate array.
    double range[2] = {0.0, 0.0};
    this->Internal->XArray->GetRange(range, xcomp);

    vtkIdType numTuples = PQ_MAX(2, this->Internal->XArray->GetNumberOfTuples());
    double delta = (range[1] - range[0])/(numTuples - 1);
    delta /= 2.0;

    this->Internal->Minimum.X = range[0]-delta;
    this->Internal->Maximum.X = range[1]+delta;
    this->Internal->XDelta = delta;

    this->Internal->YArray->GetRange(range, ycomp);
    this->Internal->Minimum.Y = range[0];
    this->Internal->Maximum.Y = range[1];
    }
  else
    {
    this->Internal->XArray = 0;
    this->Internal->YArray = 0;
    this->Internal->XDelta = 0;
    this->Internal->XArrayComponent = -1;
    this->Internal->YArrayComponent = -1;

    // No data, just show empty chart.
    this->Internal->Minimum.Y = 0;
    this->Internal->Maximum.Y = 0;
    this->Internal->Minimum.X = 0;
    this->Internal->Maximum.X = 0;
    }

  emit this->histogramReset();
}


