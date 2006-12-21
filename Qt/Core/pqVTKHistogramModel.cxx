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

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkRectilinearGrid.h"
#include "vtkIntArray.h"


class pqVTKHistogramModelInternal
{
public:
  pqVTKHistogramModelInternal();
  ~pqVTKHistogramModelInternal() {}

  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp MTime;
};


//----------------------------------------------------------------------------
pqVTKHistogramModelInternal::pqVTKHistogramModelInternal()
  : Minimum(), Maximum()
{
}


//----------------------------------------------------------------------------
pqVTKHistogramModel::pqVTKHistogramModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqVTKHistogramModelInternal();
  this->Data = 0;
}

pqVTKHistogramModel::~pqVTKHistogramModel()
{
  delete this->Internal;
  if(this->Data)
    {
    this->Data->Delete();
    }
}

int pqVTKHistogramModel::getNumberOfBins() const
{
  if (this->Data)
    {
    vtkIntArray *const values = vtkIntArray::SafeDownCast(
      this->Data->GetCellData()->GetArray("bin_values"));
    if(values && values->GetNumberOfComponents() == 1)
      {
      return values->GetNumberOfTuples();
      }
    }

  return 0;
}

void pqVTKHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  if (this->Data)
    {
    vtkIntArray *const values = vtkIntArray::SafeDownCast(
      this->Data->GetCellData()->GetArray("bin_values"));
    if(values && values->GetNumberOfComponents() == 1 && index >= 0 &&
      index < values->GetNumberOfTuples())
      {
      bin = static_cast<double>(values->GetValue(index));
      }
    }
}

void pqVTKHistogramModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqVTKHistogramModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqVTKHistogramModel::updateData(vtkDataObject* d)
{
  this->updateData(vtkRectilinearGrid::SafeDownCast(d));
}

void pqVTKHistogramModel::updateData(vtkRectilinearGrid *data)
{
  if (this->Data == data)
    {
    this->update();
    return;
    }


  // Release the reference to the old data if there is any.
  if(this->Data)
    {
    this->Data->Delete();
    this->Data = 0;
    this->Internal->Minimum.X = (int)0;
    this->Internal->Maximum.X = (int)0;
    this->Internal->Minimum.Y = (int)0;
    this->Internal->Maximum.Y = (int)0;
    }

  // Keep a reference to the new data.
  this->Data = data;
  if (this->Data)
    {
    this->Data->Register(0);
    }

  this->Internal->MTime.Modified();
  this->update();
}

void pqVTKHistogramModel::update()
{
  if ( (this->Internal->MTime > this->Internal->LastUpdateTime) ||
    (this->Data && this->Data->GetMTime() > this->Internal->LastUpdateTime))
    {
    this->forceUpdate();
    }
}

void pqVTKHistogramModel::forceUpdate()
{
  if (this->Data)
    {
    // Get the overall range for the histogram. The bin ranges are
    // stored in the x coordinate array.
    vtkDoubleArray *const extents = vtkDoubleArray::SafeDownCast(
      this->Data->GetXCoordinates());
    if(!extents || extents->GetNumberOfComponents() != 1)
      {
      qWarning("Unrecognized histogram extent data. The histogram model expects "
        "a double array of tuples with one component.");
      }
    else if(extents->GetNumberOfTuples() < 2)
      {
      qWarning("The histogram range must have at least two values.");
      }
    else
      {
      this->Internal->Minimum.X = extents->GetValue(0);
      this->Internal->Maximum.X = extents->GetValue(
        extents->GetNumberOfTuples() - 1);

      // Search through the bin values to find the y-axis range.
      vtkIntArray *const values = vtkIntArray::SafeDownCast(
        this->Data->GetCellData()->GetArray("bin_values"));
      if(!values || values->GetNumberOfComponents() != 1)
        {
        qWarning("Unrecognized histogram data. The histogram model expects "
          "an unsigned long array of tuples with one component.");
        }
      else
        {
        unsigned long min = 0;
        unsigned long max = 0;
        unsigned long value = 0;
        for(int i = 0; i != values->GetNumberOfTuples(); ++i)
          {
          value = values->GetValue(i);
          if(i == 0)
            {
            min = value;
            max = value;
            }
          else if(value < min)
            {
            min = value;
            }
          else if(value > max)
            {
            max = value;
            }
          }

        this->Internal->Minimum.Y = static_cast<double>(min);
        this->Internal->Maximum.Y = static_cast<double>(max);
        }
      }
    }
  else
    {
    this->Internal->Minimum.Y = 0;
    this->Internal->Maximum.Y = 0;
    this->Internal->Minimum.X = 0;
    this->Internal->Maximum.X = 0;
    }

  this->Internal->LastUpdateTime.Modified();
  emit this->resetBinValues();
}
