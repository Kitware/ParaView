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
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkInversedArithmeticAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkHarmonicMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->NumberOfAccumulatedData / this->Accumulators[0]->GetValue();
}

//----------------------------------------------------------------------------
void vtkHarmonicMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
