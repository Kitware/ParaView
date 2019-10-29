/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantileArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
void vtkQuantileArrayMeasurement::ShallowCopy(vtkDataObject* o)
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
void vtkQuantileArrayMeasurement::DeepCopy(vtkDataObject* o)
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
