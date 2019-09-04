/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedianArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMedianArrayMeasurement.h"

#include "vtkMedianAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkMedianArrayMeasurement);

//----------------------------------------------------------------------------
vtkMedianArrayMeasurement::vtkMedianArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkMedianArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >= vtkMedianArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkMedianArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkMedianArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkMedianArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkMedianArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() && accumulators[0] && "input accumulator is not allocated");
  vtkMedianAccumulator* medianAccumulator = vtkMedianAccumulator::SafeDownCast(accumulators[0]);
  assert(medianAccumulator &&
    "input accumulator has the wrong type. It should be of type vtkMedianAccumulator");
  return medianAccumulator->GetValue();
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkMedianArrayMeasurement::NewAccumulatorInstances() const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkMedianArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkMedianAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkMedianArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() && this->Accumulators[0])
  {
    os << indent << *(this->Accumulators[0]) << std::endl;
  }
  else
  {
    os << indent << "Missing vtkMedianAccumulator" << std::endl;
  }
}
