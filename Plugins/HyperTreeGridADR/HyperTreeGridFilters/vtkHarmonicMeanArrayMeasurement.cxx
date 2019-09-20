/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHarmonicMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHarmonicMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHarmonicMeanArrayMeasurement);
vtkArrayMeasurementMacro(vtkHarmonicMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkHarmonicMeanArrayMeasurement::vtkHarmonicMeanArrayMeasurement()
{
  this->Accumulators = vtkHarmonicMeanArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
bool vtkHarmonicMeanArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }

  assert(accumulators && "input accumulator is not allocated");

  vtkArithmeticAccumulator<vtkInverseFunctor>* acc =
    vtkArithmeticAccumulator<vtkInverseFunctor>::SafeDownCast(accumulators[0]);

  assert(this->Accumulators[0]->HasSameParameters(acc) &&
    "input accumulators are of wrong type or have wrong parameters");

  value = totalWeight / acc->GetValue();
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkHarmonicMeanArrayMeasurement::NewAccumulators()
{
  return { vtkArithmeticAccumulator<vtkInverseFunctor>::New() };
}
