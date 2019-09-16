/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArithmeticMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkArithmeticMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkArithmeticMeanArrayMeasurement::vtkArithmeticMeanArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkArithmeticMeanArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >=
    vtkArithmeticMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkArithmeticMeanArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkArithmeticMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkArithmeticMeanArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkArithmeticMeanArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() && accumulators[0] && "input accumulator is not allocated");
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulators[0]);
  assert(arithmeticAccumulator &&
    "input accumulator has the wrong type. It should be of type vtkArithmeticAccumulator");
  return arithmeticAccumulator->GetValue() / numberOfAccumulatedData;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkArithmeticMeanArrayMeasurement::NewAccumulatorInstances()
  const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkArithmeticMeanArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkArithmeticAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkArithmeticMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() && this->Accumulators[0])
  {
    os << indent << *(this->Accumulators[0]) << std::endl;
  }
  else
  {
    os << indent << "Missing vtkArithmeticAccumulator" << std::endl;
  }
}
