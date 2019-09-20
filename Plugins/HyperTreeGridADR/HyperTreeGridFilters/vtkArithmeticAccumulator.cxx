/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArithmeticAccumulator.h"

#include "vtkFunctionOfXList.h"
#include "vtkObjectFactory.h"

#include <functional>

vtkStandardNewMacro(vtkArithmeticAccumulator);

//----------------------------------------------------------------------------
std::unordered_map<double (*)(double), std::string> vtkArithmeticAccumulator::FunctionName;

//----------------------------------------------------------------------------
vtkArithmeticAccumulator::vtkArithmeticAccumulator()
  : Value(0.0)
  , FunctionOfX(VTK_FUNC_X)
{
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulator);
  assert(arithmeticAccumulator && "Cannot accumulate different accumulators");
  this->Value += arithmeticAccumulator->GetValue();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(double value, double weight)
{
  this->Value += weight * this->FunctionOfX(value);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Initialize()
{
  this->Value = 0.0;
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkArithmeticAccumulator::GetValue() const
{
  return this->Value;
}

//----------------------------------------------------------------------------
const std::function<double(double)>& vtkArithmeticAccumulator::GetFunctionOfX() const
{
  return this->FunctionOfX;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::SetFunctionOfX(double (*const f)(double), const std::string& name)
{
  this->FunctionOfX = f;
  vtkArithmeticAccumulator::FunctionName[f] = std::move(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::SetFunctionOfX(
  const std::function<double(double)>& f, const std::string& name)
{
  this->SetFunctionOfX(*(f.target<double (*)(double)>()), name);
}

//----------------------------------------------------------------------------
bool vtkArithmeticAccumulator::HasSameParameters(vtkAbstractAccumulator* accumulator) const
{
  vtkArithmeticAccumulator* acc = vtkArithmeticAccumulator::SafeDownCast(accumulator);
  // We compare the pointer on the function used.
  return acc &&
    *(this->FunctionOfX.target<double (*)(double)>()) ==
    *(acc->GetFunctionOfX().target<double (*)(double)>());
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::ShallowCopy(vtkDataObject* accumulator)
{
  this->Superclass::ShallowCopy(accumulator);
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulator);
  if (arithmeticAccumulator)
  {
    this->Value = arithmeticAccumulator->GetValue();
    this->FunctionOfX = arithmeticAccumulator->GetFunctionOfX();
  }
  else
  {
    vtkWarningMacro(<< "Could not ShallowCopy " << arithmeticAccumulator->GetClassName() << " to "
                    << this->GetClassName());
  }
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::DeepCopy(vtkDataObject* accumulator)
{
  this->Superclass::DeepCopy(accumulator);
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulator);
  if (arithmeticAccumulator)
  {
    this->Value = arithmeticAccumulator->GetValue();
    this->FunctionOfX = arithmeticAccumulator->GetFunctionOfX();
  }
  else
  {
    vtkWarningMacro(<< "Could not DeepCopy " << arithmeticAccumulator->GetClassName() << " to "
                    << this->GetClassName());
  }
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FunctionOfX: "
     << vtkArithmeticAccumulator::FunctionName[*(this->FunctionOfX.target<double (*)(double)>())]
     << std::endl;
}
