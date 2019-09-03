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
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkArithmeticAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkArithmeticMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue() / this->NumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
void vtkArithmeticMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
