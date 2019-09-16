/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEntropyArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkEntropyAccumulator.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkEntropyArrayMeasurement);

//----------------------------------------------------------------------------
vtkEntropyArrayMeasurement::vtkEntropyArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkEntropyArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >=
    vtkEntropyArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkEntropyArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkEntropyArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkEntropyArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkEntropyArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() == 2 && accumulators[0] && accumulators[1] &&
    "input accumulator is not allocated");
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulators[0]);
  vtkEntropyAccumulator* entropyAccumulator = vtkEntropyAccumulator::SafeDownCast(accumulators[1]);
  assert(arithmeticAccumulator && entropyAccumulator &&
    "input accumulators have the wrong type. One should be of type vtkArithmeticAccumulator and "
    "the other of type vtkSquaredArithmeticAccumulator");

  // x_i : input
  // p_i = f_i / n
  // entropy = 1/n sum_i (x_i log(x_i) + x_i log(n))
  return (arithmeticAccumulator->GetValue() * std::log(numberOfAccumulatedData) +
           entropyAccumulator->GetValue()) /
    numberOfAccumulatedData;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkEntropyArrayMeasurement::NewAccumulatorInstances() const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkEntropyArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkArithmeticAccumulator::New();
  accumulators[1] = vtkEntropyAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkEntropyArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() == vtkEntropyArrayMeasurement::NumberOfAccumulators &&
    this->Accumulators[0] && this->Accumulators[1])
  {
    os << indent << *(this->Accumulators[0]) << std::endl;
    os << indent << *(this->Accumulators[1]) << std::endl;
  }
  else
  {
    os << indent << "Missing vtkArithmeticAccumulator or vtkSquaredArithmeticAccumulator"
       << std::endl;
  }
}
