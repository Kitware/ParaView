/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHarmonicMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHarmonicMeanArrayMeasurement.h"

#include "vtkInversedArithmeticAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkHarmonicMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkHarmonicMeanArrayMeasurement::vtkHarmonicMeanArrayMeasurement()
{
  this->Accumulators = this->NewAccumulatorInstances();
}

//----------------------------------------------------------------------------
bool vtkHarmonicMeanArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData >=
    vtkHarmonicMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
vtkIdType vtkHarmonicMeanArrayMeasurement::GetMinimumNumberOfAccumulatedData() const
{
  return vtkHarmonicMeanArrayMeasurement::MinimumNumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
double vtkHarmonicMeanArrayMeasurement::Measure() const
{
  return this->Measure(this->Accumulators, this->NumberOfAccumulatedData);
}

//----------------------------------------------------------------------------
double vtkHarmonicMeanArrayMeasurement::Measure(
  const std::vector<vtkAbstractAccumulator*>& accumulators, vtkIdType numberOfAccumulatedData) const
{
  assert(accumulators.size() && accumulators[0] && "input accumulator is not allocated");
  vtkInversedArithmeticAccumulator* inversedArithmeticAccumulator =
    vtkInversedArithmeticAccumulator::SafeDownCast(accumulators[0]);
  assert(inversedArithmeticAccumulator &&
    "input accumulator has the wrong type. It should be of type vtkInversedArithmeticAccumulator");
  return numberOfAccumulatedData / inversedArithmeticAccumulator->GetValue();
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkHarmonicMeanArrayMeasurement::NewAccumulatorInstances()
  const
{
  std::vector<vtkAbstractAccumulator*> accumulators(
    vtkHarmonicMeanArrayMeasurement::NumberOfAccumulators);
  accumulators[0] = vtkInversedArithmeticAccumulator::New();
  return accumulators;
}

//----------------------------------------------------------------------------
void vtkHarmonicMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Accumulators.size() && this->Accumulators[0])
  {
    os << indent << *(this->Accumulators[0]) << std::endl;
  }
  else
  {
    os << indent << "Missing vtkInversedArithmeticAccumulator" << std::endl;
  }
}
