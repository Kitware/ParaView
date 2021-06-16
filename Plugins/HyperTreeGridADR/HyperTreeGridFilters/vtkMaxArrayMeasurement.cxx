/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaxArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMaxArrayMeasurement.h"

#include "vtkFunctor.h"
#include "vtkMaxAccumulator.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkMaxArrayMeasurement);
vtkArrayMeasurementMacro(vtkMaxArrayMeasurement);

//----------------------------------------------------------------------------
vtkMaxArrayMeasurement::vtkMaxArrayMeasurement()
{
  this->Accumulators = vtkMaxArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
void vtkMaxArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkMaxArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }
  assert(accumulators && "input accumulator is not allocated");

  vtkMaxAccumulator* acc = vtkMaxAccumulator::SafeDownCast(accumulators[0]);

  assert(this->Accumulators[0]->HasSameParameters(acc) &&
    "input accumulators are of wrong type or have wrong parameters");

  value = acc->GetValue();
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkMaxArrayMeasurement::NewAccumulators()
{
  return { vtkMaxAccumulator::New() };
}
