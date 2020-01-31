/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeometricMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkGeometricMeanArrayMeasurement);
vtkArrayMeasurementMacro(vtkGeometricMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkGeometricMeanArrayMeasurement::vtkGeometricMeanArrayMeasurement()
{
  this->Accumulators = vtkGeometricMeanArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
bool vtkGeometricMeanArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }

  assert(accumulators && "input accumulator is not allocated");

  vtkArithmeticAccumulator<vtkLogFunctor>* acc =
    vtkArithmeticAccumulator<vtkLogFunctor>::SafeDownCast(accumulators[0]);

  assert(this->Accumulators[0]->HasSameParameters(acc) &&
    "input accumulators are of wrong type or have wrong parameters");

  value = std::exp(acc->GetValue() / totalWeight);
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkGeometricMeanArrayMeasurement::NewAccumulators()
{
  return { vtkArithmeticAccumulator<vtkLogFunctor>::New() };
}
