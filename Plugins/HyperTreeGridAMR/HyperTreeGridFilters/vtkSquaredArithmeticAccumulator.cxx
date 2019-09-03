/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquaredArithmeticAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSquaredArithmeticAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkSquaredArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkSquaredArithmeticAccumulator::vtkSquaredArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkSquaredArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Add(double value)
{
  this->Value += value * value;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
