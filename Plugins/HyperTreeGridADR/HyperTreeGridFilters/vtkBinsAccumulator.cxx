/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinsAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNEs FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBinsAccumulator.h"

#include "vtkFunctionOfXList.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <memory>
#include <string>

vtkStandardNewMacro(vtkBinsAccumulator);

//----------------------------------------------------------------------------
std::unordered_map<double (*)(double), std::string> vtkBinsAccumulator::FunctionName;

//----------------------------------------------------------------------------
vtkBinsAccumulator::vtkBinsAccumulator()
  : Bins(std::make_shared<BinsType>())
  , FunctionOfW(VTK_FUNC_X)
  , DiscretizationStep(0.0)
  , Value(0.0)
{
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Bins: ";
  for (auto& bin : *(this->Bins))
  {
    os << indent << "[" << bin.first << ", " << bin.second << "] ";
  }
  os << indent << std::endl;
  os << indent << "DiscretizationStep: " << this->DiscretizationStep << std::endl;
  os << indent << "FunctionOfW: "
     << vtkBinsAccumulator::FunctionName[*(this->FunctionOfW.target<double (*)(double)>())]
     << std::endl;
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  vtkBinsAccumulator* binAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  assert(binAccumulator && "accumulator not of type vtkBinsAccumulator, cannot Add");
  for (const auto& bin : *(binAccumulator->GetBins()))
  {
    auto it = this->Bins->find(bin.first);
    if (it == this->Bins->end())
    {
      (*this->Bins)[bin.first] = bin.second;
      this->Value += this->FunctionOfW(bin.second);
    }
    else
    {
      this->Value -= this->FunctionOfW(it->second);
      it->second += bin.second;
      this->Value += this->FunctionOfW(it->second);
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::Add(double value, double weight)
{
  auto it = this->Bins->find(static_cast<long long>(value / this->DiscretizationStep));
  if (it == this->Bins->end())
  {
    (*this->Bins)[static_cast<long long>(value / this->DiscretizationStep)] = weight;
    this->Value += this->FunctionOfW(weight);
  }
  else
  {
    this->Value -= this->FunctionOfW(it->second);
    it->second += weight;
    this->Value += this->FunctionOfW(it->second);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::Initialize()
{
  this->Value = 0.0;
  this->FunctionOfW = VTK_FUNC_X;
  this->DiscretizationStep = 0.0;
  this->Bins->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
const vtkBinsAccumulator::BinsPointer vtkBinsAccumulator::GetBins() const
{
  return this->Bins;
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::SetDiscretizationStep(double discretizationStep)
{
  if (this->Bins->size())
  {
    vtkWarningMacro(<< "Setting DiscretizationStep while Bins are not empty");
  }
  this->DiscretizationStep = discretizationStep;
  this->Modified();
}

//----------------------------------------------------------------------------
const std::function<double(double)>& vtkBinsAccumulator::GetFunctionOfW() const
{
  return this->FunctionOfW;
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::SetFunctionOfW(double (*const f)(double), const std::string& name)
{
  this->FunctionOfW = f;
  vtkBinsAccumulator::FunctionName[f] = std::move(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::SetFunctionOfW(
  const std::function<double(double)>& f, const std::string& name)
{
  this->SetFunctionOfW(*(f.target<double (*)(double)>()), name);
}

//----------------------------------------------------------------------------
double vtkBinsAccumulator::GetValue() const
{
  return this->Value;
}

//----------------------------------------------------------------------------
bool vtkBinsAccumulator::HasSameParameters(vtkAbstractAccumulator* accumulator) const
{
  vtkBinsAccumulator* acc = vtkBinsAccumulator::SafeDownCast(accumulator);
  // We compare the pointer on the function used.
  return acc && acc->DiscretizationStep == this->DiscretizationStep &&
    *(this->FunctionOfW.target<double (*)(double)>()) ==
    *(acc->GetFunctionOfW().target<double (*)(double)>());
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::ShallowCopy(vtkDataObject* accumulator)
{
  this->Superclass::ShallowCopy(accumulator);
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  if (binsAccumulator)
  {
    this->Bins = binsAccumulator->GetBins();
    this->FunctionOfW = binsAccumulator->GetFunctionOfW();
    this->DiscretizationStep = binsAccumulator->GetDiscretizationStep();
  }
  else
  {
    this->Bins = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkBinsAccumulator::DeepCopy(vtkDataObject* accumulator)
{
  this->Superclass::DeepCopy(accumulator);
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  if (binsAccumulator)
  {
    const BinsPointer& bins = binsAccumulator->GetBins();
    this->Bins = std::make_shared<BinsType>(bins->cbegin(), bins->cend());
    this->FunctionOfW = binsAccumulator->GetFunctionOfW();
    this->DiscretizationStep = binsAccumulator->GetDiscretizationStep();
  }
  else
  {
    this->Bins = nullptr;
  }
}
