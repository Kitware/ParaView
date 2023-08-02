// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
