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

vtkStandardNewMacro(vtkGeometricMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkGeometricMeanArrayMeasurement::vtkGeometricMeanArrayMeasurement()
{
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkGeometricAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkGeometricMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue() / this->NumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
void vtkGeometricMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
