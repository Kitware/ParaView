/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEntropyArrayMeasurement.h"

#include "vtkBinsAccumulator.h"
#include "vtkFunctionOfXList.h"
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
bool vtkEntropyArrayMeasurement::Measure(vtkAbstractAccumulator** accumulators,
  vtkIdType numberOfAccumulatedData, double totalWeight, double& value)
{
  if (!this->CanMeasure(numberOfAccumulatedData, totalWeight))
  {
    return false;
  }
  assert(accumulators && "input accumulator is not allocated");

  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(accumulators[0]);

  assert(binsAccumulator && "input accumulator is of wrong type");

  value = binsAccumulator->GetValue() / totalWeight + std::log(totalWeight);

  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*> vtkEntropyArrayMeasurement::NewAccumulators()
{
  vtkBinsAccumulator* acc = vtkBinsAccumulator::New();
  acc->SetFunctionOfW(vtkValueComaNameMacro(VTK_FUNC_NXLOGX));
  std::vector<vtkAbstractAccumulator*> accumulators{ acc };
  return accumulators;
}

//----------------------------------------------------------------------------
double vtkEntropyArrayMeasurement::GetDiscretizationStep() const
{
  assert(this->Accumulators.size() && "No accumulator in vtkEntropyArrayMeasurement");
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(this->Accumulators[0]);
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
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(this->Accumulators[0]);
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
