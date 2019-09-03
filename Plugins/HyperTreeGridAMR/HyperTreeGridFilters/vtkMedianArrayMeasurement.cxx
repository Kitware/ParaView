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
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkMedianAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkMedianArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue();
}

//----------------------------------------------------------------------------
void vtkMedianArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
