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
#include "vtkDoubleArray.h"
#include "vtkSmartPointer.h"

#include <math.h>


class pqVTKLineChartSeriesInternal
{
public:
  pqVTKLineChartSeriesInternal();
  ~pqVTKLineChartSeriesInternal() {}

  vtkSmartPointer<vtkDataArray> XArray;
  vtkSmartPointer<vtkDataArray> YArray;
  QList<int> BreakList;
};


//-----------------------------------------------------------------------------
pqVTKLineChartSeriesInternal::pqVTKLineChartSeriesInternal()
  : BreakList()
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

bool pqVTKLineChartSeries::getPoint(int sequence, int index,
    pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->getNumberOfPoints(sequence))
    {
    coord.X = this->Internal->XArray->GetTuple1(index);
    coord.Y = this->Internal->YArray->GetTuple1(index);

    return this->Internal->BreakList.contains(index);
    }

  return false;
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
    vtkDataArray *yArray, vtkDataArray *mask, int xComponent, int yComponent)
{
  this->Internal->XArray = 0;
  this->Internal->YArray = 0;
  this->Internal->BreakList.clear();
  if(xArray && yArray)
    {
    this->Internal->XArray = xArray;
    if(this->Internal->XArray->GetNumberOfComponents() > 1)
      {
      this->Internal->XArray = this->createArray(xArray, xComponent);
      }

    if(this->Internal->XArray)
      {
      this->Internal->YArray = yArray;
      if(this->Internal->YArray->GetNumberOfComponents() > 1)
        {
        this->Internal->YArray = this->createArray(yArray, yComponent);
        }

      if(!this->Internal->YArray)
        {
        this->Internal->XArray = 0;
        }
      }

    // If there is a mask array, apply it to the data.
    if(mask && this->Internal->XArray)
      {
      // Get the number of valid points.
      vtkIdType i = 0;
      vtkIdType numValid = 0;
      vtkIdType maskLength = mask->GetNumberOfTuples();
      for( ; i < mask->GetNumberOfTuples(); i++)
        {
        if(mask->GetTuple1(i) != 0)
          {
          numValid++;
          }
        }

      if(numValid < maskLength)
        {
        // Allocate new arrays.
        vtkSmartPointer<vtkDoubleArray> newXArray =
            vtkSmartPointer<vtkDoubleArray>::New();
        newXArray->SetNumberOfComponents(1);
        newXArray->SetNumberOfTuples(numValid);
        vtkSmartPointer<vtkDoubleArray> newYArray =
            vtkSmartPointer<vtkDoubleArray>::New();
        newYArray->SetNumberOfComponents(1);
        newYArray->SetNumberOfTuples(numValid);

        // Copy the valid points to the new arrays.
        vtkIdType j = 0;
        bool inBreak = false;
        vtkIdType numPts = this->Internal->XArray->GetNumberOfTuples();
        for(i = 0; i < maskLength && i < numPts; i++)
          {
          if(mask->GetTuple1(i) == 0)
            {
            inBreak = true;
            }
          else
            {
            if(inBreak && j != 0)
              {
              inBreak = false;
              this->Internal->BreakList.append((int)j);
              }

            newXArray->SetTuple1(j, this->Internal->XArray->GetTuple1(i));
            newYArray->SetTuple1(j, this->Internal->YArray->GetTuple1(i));
            j++;
            }
          }

        this->Internal->XArray = newXArray;
        this->Internal->YArray = newYArray;
        }
      }
    }

  this->resetSeries();
}

vtkSmartPointer<vtkDataArray> pqVTKLineChartSeries::createArray(
    vtkDataArray *array, int component)
{
  if(component == -1)
    {
    return pqVTKLineChartSeries::createMagnitudeArray(array);
    }
  else if(component == -2)
    {
    return pqVTKLineChartSeries::createDistanceArray(array);
    }
  else if(component < 0 || !array)
    {
    return 0;
    }

  int numComp = array->GetNumberOfComponents();
  if(component >= numComp)
    {
    return 0;
    }
  else if(numComp == 1)
    {
    return array;
    }

  vtkIdType numPts = array->GetNumberOfTuples();
  vtkSmartPointer<vtkDoubleArray> newArray =
      vtkSmartPointer<vtkDoubleArray>::New();
  newArray->SetNumberOfComponents(1);
  newArray->SetNumberOfTuples(numPts);

  double *tupleData = new double[numComp];
  for(vtkIdType i = 0; i < numPts; i++)
    {
    array->GetTuple(i, tupleData);
    newArray->SetTuple1(i, tupleData[component]);
    }

  delete [] tupleData;
  return newArray;
}

vtkSmartPointer<vtkDataArray> pqVTKLineChartSeries::createMagnitudeArray(
    vtkDataArray *array)
{
  if(!array || array->GetNumberOfComponents() < 2)
    {
    return array;
    }

  vtkIdType numPts = array->GetNumberOfTuples();
  vtkSmartPointer<vtkDoubleArray> magnitude =
      vtkSmartPointer<vtkDoubleArray>::New();
  magnitude->SetNumberOfComponents(1);
  magnitude->SetNumberOfTuples(numPts);

  int numComp = array->GetNumberOfComponents();
  double *tupleData = new double[numComp];
  for(vtkIdType i = 0; i < numPts; i++)
    {
    double sum = 0.0;
    array->GetTuple(i, tupleData);
    for(int j = 0; j < numComp; j++)
      {
      sum += tupleData[j] * tupleData[j];
      }

    if(sum > 0.0)
      {
      sum = sqrt(sum);
      }

    magnitude->SetTuple1(i, sum);
    }

  delete [] tupleData;
  return magnitude;
}

vtkSmartPointer<vtkDataArray> pqVTKLineChartSeries::createDistanceArray(
    vtkDataArray *array)
{
  if(!array || array->GetNumberOfComponents() < 1)
    {
    return array;
    }

  vtkIdType numPts = array->GetNumberOfTuples();
  vtkSmartPointer<vtkDoubleArray> distance =
      vtkSmartPointer<vtkDoubleArray>::New();
  distance->SetNumberOfComponents(1);
  distance->SetNumberOfTuples(numPts);

  int numComp = array->GetNumberOfComponents();
  double *current = new double[numComp];
  double *previous = new double[numComp];
  if(numPts > 0)
    {
    distance->SetTuple1(0, 0.0);
    }

  double total = 0.0;
  for(vtkIdType i = 1; i < numPts; i++)
    {
    double sum = 0.0;
    array->GetTuple(i, current);
    array->GetTuple(i - 1, previous);
    for(int j = 0; j < numComp; j++)
      {
      current[j] = current[j] - previous[j];
      if(numComp == 1)
        {
        sum = current[j];
        }
      else
        {
        sum += current[j] * current[j];
        }
      }

    if(numComp > 1 && sum > 0.0)
      {
      sum = sqrt(sum);
      }

    total += sum;
    distance->SetTuple1(i, total);
    }

  delete [] current;
  delete [] previous;
  return distance;
}


