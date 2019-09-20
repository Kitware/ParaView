/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticMeanArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArithmeticMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkFunctionOfXList.h"
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
bool vtkArithmeticMeanArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
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

  value = acc->GetValue() / totalWeight;
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkArithmeticMeanArrayMeasurement::NewAccumulators()
{
  vtkArithmeticAccumulator* acc = vtkArithmeticAccumulator::New();
  acc->SetFunctionOfX(vtkValueComaNameMacro(VTK_FUNC_X));
  std::vector<vtkAbstractAccumulator*> accumulators{ acc };
  return accumulators;
}
