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
#include "vtkFunctionOfXList.h"
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

  vtkArithmeticAccumulator* acc = vtkArithmeticAccumulator::SafeDownCast(accumulators[0]);

  assert(this->Accumulators[0]->HasSameParameters(acc) &&
    "input accumulators are of wrong type or have wrong parameters");

  value = totalWeight / acc->GetValue();
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkHarmonicMeanArrayMeasurement::NewAccumulators()
{
  vtkArithmeticAccumulator* acc = vtkArithmeticAccumulator::New();
  acc->SetFunctionOfX(vtkValueComaNameMacro(VTK_FUNC_1_X));
  std::vector<vtkAbstractAccumulator*> accumulators{ acc };
  return accumulators;
}
