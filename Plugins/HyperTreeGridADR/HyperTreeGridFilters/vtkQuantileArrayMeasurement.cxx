// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkQuantileArrayMeasurement.h"

#include "vtkObjectFactory.h"
#include "vtkQuantileAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkQuantileArrayMeasurement);
vtkArrayMeasurementMacro(vtkQuantileArrayMeasurement);

//----------------------------------------------------------------------------
vtkQuantileArrayMeasurement::vtkQuantileArrayMeasurement()
{
  this->Accumulators = vtkQuantileArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
void vtkQuantileArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkQuantileArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!vtkQuantileArrayMeasurement::IsMeasurable(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }

  assert(accumulators && "input accumulator is not allocated");

  vtkQuantileAccumulator* quantileAccumulator =
    vtkQuantileAccumulator::SafeDownCast(accumulators[0]);

  assert(quantileAccumulator && "input accumulator is of wrong type");

  value = quantileAccumulator->GetValue();
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkQuantileArrayMeasurement::NewAccumulators()
{
  return std::vector<vtkAbstractAccumulator*>{ vtkQuantileAccumulator::New() };
}

//----------------------------------------------------------------------------
double vtkQuantileArrayMeasurement::GetPercentile() const
{
  assert(this->Accumulators.size() && "Accumulators not set");
  vtkQuantileAccumulator* acc = vtkQuantileAccumulator::SafeDownCast(this->Accumulators[0]);
  return acc->GetPercentile();
}

//----------------------------------------------------------------------------
void vtkQuantileArrayMeasurement::SetPercentile(double percentile)
{
  assert(this->Accumulators.size() && "Accumulators not set");
  vtkQuantileAccumulator* acc = vtkQuantileAccumulator::SafeDownCast(this->Accumulators[0]);
  acc->SetPercentile(percentile);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuantileArrayMeasurement::ShallowCopy(vtkObject* o)
{
  this->Superclass::ShallowCopy(o);
  vtkQuantileArrayMeasurement* quantileArrayMeasurement =
    vtkQuantileArrayMeasurement::SafeDownCast(o);
  if (quantileArrayMeasurement)
  {
    this->SetPercentile(quantileArrayMeasurement->GetPercentile());
  }
  else
  {
    vtkWarningMacro(<< "Trying to shallow copy a " << o->GetClassName()
                    << " into a vtkQuantileArrayMeasurement");
  }
}

//----------------------------------------------------------------------------
void vtkQuantileArrayMeasurement::DeepCopy(vtkObject* o)
{
  this->Superclass::DeepCopy(o);
  vtkQuantileArrayMeasurement* quantileArrayMeasurement =
    vtkQuantileArrayMeasurement::SafeDownCast(o);
  if (quantileArrayMeasurement)
  {
    this->SetPercentile(quantileArrayMeasurement->GetPercentile());
  }
  else
  {
    vtkWarningMacro(<< "Trying to deep copy a " << o->GetClassName()
                    << " into a vtkQuantileArrayMeasurement");
  }
}
