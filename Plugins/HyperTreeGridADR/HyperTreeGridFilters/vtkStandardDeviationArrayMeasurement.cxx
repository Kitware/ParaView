/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStandardDeviationArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStandardDeviationArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkSquaredArithmeticAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkStandardDeviationArrayMeasurement);

//----------------------------------------------------------------------------
vtkStandardDeviationArrayMeasurement::vtkStandardDeviationArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkStandardDeviationArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >=
    vtkStandardDeviationArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkStandardDeviationArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkStandardDeviationArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkStandardDeviationArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkStandardDeviationArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() == 2 && accumulators[0] && accumulators[1] &&
    "input accumulator is not allocated");
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulators[0]);
  vtkSquaredArithmeticAccumulator* squaredArithmeticAccumulator =
    vtkSquaredArithmeticAccumulator::SafeDownCast(accumulators[1]);
  assert(arithmeticAccumulator && squaredArithmeticAccumulator &&
    "input accumulators have the wrong type. One should be of type vtkArithmeticAccumulator and "
    "the other of type vtkSquaredArithmeticAccumulator");

  double mean = arithmeticAccumulator->GetValue() / numberOfAccumulatedData;
  // std = sqrt(sum_i (x_i - mean)^2 / (n-1))
  //     = sqrt(sum_i (x_i^2  - 2*x_i*mean + n*mean^2) / (n-1))
  return std::sqrt(
    (squaredArithmeticAccumulator->GetValue() - 2 * arithmeticAccumulator->GetValue() * mean +
      mean * mean * numberOfAccumulatedData) /
    (numberOfAccumulatedData - 1));
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkStandardDeviationArrayMeasurement::NewAccumulatorInstances()
  const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkStandardDeviationArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkArithmeticAccumulator::New();
  accumulators[1] = vtkSquaredArithmeticAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkStandardDeviationArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() == vtkStandardDeviationArrayMeasurement::NumberOfAccumulators &&
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
