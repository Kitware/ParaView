/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInversedArithmeticAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInversedArithmeticAccumulator.h"

#include "vtkObjectFactory.h"
#include "vtkSetGet.h"

#include <cassert>

vtkStandardNewMacro(vtkInversedArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkInversedArithmeticAccumulator::vtkInversedArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkInversedArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Add(double value)
{
  if (value == 0)
  {
    vtkErrorMacro("Cannot add null values into inversed arithmetic accumulator");
  }
  this->Value += 1 / value;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
