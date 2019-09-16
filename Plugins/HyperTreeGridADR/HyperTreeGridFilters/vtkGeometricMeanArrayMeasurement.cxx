/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeometricMeanArrayMeasurement.h"

#include "vtkGeometricAccumulator.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkGeometricMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkGeometricMeanArrayMeasurement::vtkGeometricMeanArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkGeometricMeanArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >=
    vtkGeometricMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkGeometricMeanArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkGeometricMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkGeometricMeanArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkGeometricMeanArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() && accumulators[0] && "input accumulator is not allocated");
  vtkGeometricAccumulator* geometricAccumulator =
    vtkGeometricAccumulator::SafeDownCast(accumulators[0]);
  assert(geometricAccumulator &&
    "input accumulator has the wrong type. It should be of type vtkGeometricAccumulator");
  return std::pow(geometricAccumulator->GetValue(), 1.0 / numberOfAccumulatedData);
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkGeometricMeanArrayMeasurement::NewAccumulatorInstances()
  const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkGeometricMeanArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkGeometricAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkGeometricMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() && this->Accumulators[0])
  {
    os << indent << *(this->Accumulators[0]) << std::endl;
  }
  else
  {
    os << indent << "Missing vtkGeometricAccumulator" << std::endl;
  }
}
