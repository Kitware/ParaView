/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStandardDeviationArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStandardDeviationArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"
#include "vtkFunctionOfXList.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkStandardDeviationArrayMeasurement);
vtkArrayMeasurementMacro(vtkStandardDeviationArrayMeasurement);

//----------------------------------------------------------------------------
vtkStandardDeviationArrayMeasurement::vtkStandardDeviationArrayMeasurement()
{
  this->Accumulators = vtkStandardDeviationArrayMeasurement::NewAccumulators();
}

//----------------------------------------------------------------------------
bool vtkStandardDeviationArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }
  assert(accumulators && "input accumulator is not allocated");

  vtkArithmeticAccumulator* identityAcc = vtkArithmeticAccumulator::SafeDownCast(
    accumulators[vtkStandardDeviationArrayMeasurement::IdentityId]);
  vtkArithmeticAccumulator* squaredAcc = vtkArithmeticAccumulator::SafeDownCast(
    accumulators[vtkStandardDeviationArrayMeasurement::SquaredId]);

  assert(this->Accumulators[vtkStandardDeviationArrayMeasurement::IdentityId]->HasSameParameters(
           identityAcc) &&
    this->Accumulators[vtkStandardDeviationArrayMeasurement::SquaredId]->HasSameParameters(
      squaredAcc) &&
    "input accumulators are of wrong type or have wrong parameters");

  double weightedMean = identityAcc->GetValue() / totalWeight;
  // std = sqrt(sum_i w_i(x_i - mean)^2 / (sum_i w_i (n-1)/n))
  //     = sqrt(sum_i (x_i^2  - 2*x_i*mean + n*mean^2) / (sum_i w_i (n-1)/n))
  value = std::sqrt((squaredAcc->GetValue() - 2 * weightedMean * identityAcc->GetValue() +
                      weightedMean * weightedMean * totalWeight) /
    (totalWeight * (numberOfAccumulatedData - 1.0) / numberOfAccumulatedData));

  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkStandardDeviationArrayMeasurement::NewAccumulators()
{
  vtkArithmeticAccumulator* identityAcc = vtkArithmeticAccumulator::New();
  vtkArithmeticAccumulator* squaredAcc = vtkArithmeticAccumulator::New();
  identityAcc->SetFunctionOfX(vtkValueComaNameMacro(VTK_FUNC_X));
  squaredAcc->SetFunctionOfX(vtkValueComaNameMacro(VTK_FUNC_X2));
  std::vector<vtkAbstractAccumulator*> accumulators{ identityAcc, squaredAcc };
  return accumulators;
}
