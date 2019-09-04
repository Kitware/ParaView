/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArithmeticAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkArithmeticAccumulator::vtkArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(double value)
{
  this->Value += value;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Initialize()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
