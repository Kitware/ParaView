// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArithmeticMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkArithmeticMeanArrayMeasurement);
vtkArrayMeasurementMacro(vtkArithmeticMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkArithmeticMeanArrayMeasurement::vtkArithmeticMeanArrayMeasurement()
{
  this->Accumulators = vtkArithmeticMeanArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
void vtkArithmeticMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkArithmeticMeanArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }
  assert(accumulators && "input accumulator is not allocated");

  vtkArithmeticAccumulator<vtkIdentityFunctor>* acc =
    vtkArithmeticAccumulator<vtkIdentityFunctor>::SafeDownCast(accumulators[0]);

  assert(this->Accumulators[0]->HasSameParameters(acc) &&
    "input accumulators are of wrong type or have wrong parameters");

  value = acc->GetValue() / totalWeight;
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkArithmeticMeanArrayMeasurement::NewAccumulators()
{
  return { vtkArithmeticAccumulator<vtkIdentityFunctor>::New() };
}
