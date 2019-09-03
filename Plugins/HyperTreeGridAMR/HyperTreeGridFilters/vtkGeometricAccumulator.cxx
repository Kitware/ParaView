/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeometricAccumulator.h"

#include "vtkObjectFactory.h"
#include "vtkSetGet.h"

#include <cassert>

vtkStandardNewMacro(vtkGeometricAccumulator);

//----------------------------------------------------------------------------
vtkGeometricAccumulator::vtkGeometricAccumulator()
{
  this->Value = 1.0;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkGeometricAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Add(accumulator->GetValue());
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Add(double value)
{
  if (value <= 0)
  {
    vtkErrorMacro(
      "Cannot add null or negative values into a geometric accumulator: value " << value);
  }
  this->Value *= value;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 1.0;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
