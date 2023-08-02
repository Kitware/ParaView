// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEntropyArrayMeasurement.h"

#include "vtkBinsAccumulator.h"
#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkEntropyArrayMeasurement);
vtkArrayMeasurementMacro(vtkEntropyArrayMeasurement);

//----------------------------------------------------------------------------
vtkEntropyArrayMeasurement::vtkEntropyArrayMeasurement()
{
  this->Accumulators = vtkEntropyArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
void vtkEntropyArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkEntropyArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }
  assert(accumulators && "input accumulator is not allocated");

  vtkBinsAccumulator<vtkEntropyFunctor>* binsAccumulator =
    vtkBinsAccumulator<vtkEntropyFunctor>::SafeDownCast(accumulators[0]);

  assert(binsAccumulator && "input accumulator is of wrong type");

  value = binsAccumulator->GetValue() / totalWeight + std::log(totalWeight);

  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkEntropyArrayMeasurement::NewAccumulators()
{
  return { vtkBinsAccumulator<vtkEntropyFunctor>::New() };
}

//----------------------------------------------------------------------------
double vtkEntropyArrayMeasurement::GetDiscretizationStep() const
{
  assert(this->Accumulators.size() && "No accumulator in vtkEntropyArrayMeasurement");
  vtkBinsAccumulator<vtkEntropyFunctor>* binsAccumulator =
    vtkBinsAccumulator<vtkEntropyFunctor>::SafeDownCast(this->Accumulators[0]);
  if (binsAccumulator)
  {
    return binsAccumulator->GetDiscretizationStep();
  }
  else
  {
    vtkWarningMacro(<< "Wrong type, accumulator " << this->Accumulators[0]->GetClassName()
                    << " instead of vtkBinsAccumulator in vtkEntropyArrayMeasurement");
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkEntropyArrayMeasurement::SetDiscretizationStep(double discretizationStep)
{
  assert(this->Accumulators.size() && "No accumulator in vtkEntropyArrayMeasurement");
  vtkBinsAccumulator<vtkEntropyFunctor>* binsAccumulator =
    vtkBinsAccumulator<vtkEntropyFunctor>::SafeDownCast(this->Accumulators[0]);
  if (binsAccumulator)
  {
    binsAccumulator->SetDiscretizationStep(discretizationStep);
    this->Modified();
  }
  else
  {
    vtkWarningMacro(<< "Wrong type, accumulator " << this->Accumulators[0]->GetClassName()
                    << " instead of vtkBinsAccumulator in vtkEntropyArrayMeasurement");
  }
}
