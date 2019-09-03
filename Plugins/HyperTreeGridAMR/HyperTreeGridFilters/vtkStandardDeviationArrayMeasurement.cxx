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
  this->Accumulators.resize(2);
  this->Accumulators[0] = vtkArithmeticAccumulator::New();
  this->Accumulators[1] = vtkSquaredArithmeticAccumulator::New();
}

//----------------------------------------------------------------------------
bool vtkStandardDeviationArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData > 1;
}

//----------------------------------------------------------------------------
double vtkStandardDeviationArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() > 1 && "No accumulator, cannot measure");
  double mean = this->Accumulators[0]->GetValue() / this->NumberOfAccumulatedData;
  // std = sqrt(sum_i (x_i - mean)^2 / (n-1))
  //     = sqrt(sum_i (x_i^2  - 2*x_i*mean + n*mean^2) / (n-1))
  return std::sqrt(
    (this->Accumulators[1]->GetValue() - 2 * this->Accumulators[0]->GetValue() * mean +
      mean * mean * this->NumberOfAccumulatedData) /
    (this->NumberOfAccumulatedData - 1));
}

//----------------------------------------------------------------------------
void vtkStandardDeviationArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
