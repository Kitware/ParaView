/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinsAccumulator.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNEs FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkBinsAccumulator_txx
#define vtkBinsAccumulator_txx

#include "vtkBinsAccumulator.h"

#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <memory>
#include <string>

template <typename FunctorT>
vtkStandardNewMacro(vtkBinsAccumulator<FunctorT>);

//----------------------------------------------------------------------------
template <typename FunctorT>
vtkBinsAccumulator<FunctorT>::vtkBinsAccumulator()
  : Bins(std::make_shared<BinsType>())
  , DiscretizationStep(0.0)
  , Value(0.0)
{
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Bins: ";
  for (auto& bin : *(this->Bins))
  {
    os << indent << "[" << bin.first << ", " << bin.second << "] ";
  }
  os << indent << std::endl;
  os << indent << "DiscretizationStep: " << this->DiscretizationStep << std::endl;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::Add(vtkAbstractAccumulator* accumulator)
{
  vtkBinsAccumulator* binAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  assert(binAccumulator && "accumulator not of type vtkBinsAccumulator, cannot Add");
  for (const auto& bin : *(binAccumulator->GetBins()))
  {
    auto it = this->Bins->find(bin.first);
    if (it == this->Bins->end())
    {
      (*this->Bins)[bin.first] = bin.second;
      this->Value += this->Functor(bin.second);
    }
    else
    {
      this->Value -= this->Functor(it->second);
      it->second += bin.second;
      this->Value += this->Functor(it->second);
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::Add(double value, double weight)
{
  auto it = this->Bins->find(static_cast<long long>(value / this->DiscretizationStep));
  if (it == this->Bins->end())
  {
    (*this->Bins)[static_cast<long long>(value / this->DiscretizationStep)] = weight;
    this->Value += this->Functor(weight);
  }
  else
  {
    this->Value -= this->Functor(it->second);
    it->second += weight;
    this->Value += this->Functor(it->second);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::Initialize()
{
  this->Value = 0.0;
  this->Functor = FunctorType();
  this->DiscretizationStep = 0.0;
  this->Bins->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
const typename vtkBinsAccumulator<FunctorT>::BinsPointer vtkBinsAccumulator<FunctorT>::GetBins()
  const
{
  return this->Bins;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::SetDiscretizationStep(double discretizationStep)
{
  if (this->Bins->size())
  {
    vtkWarningMacro(<< "Setting DiscretizationStep while Bins are not empty");
  }
  this->DiscretizationStep = discretizationStep;
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
const FunctorT& vtkBinsAccumulator<FunctorT>::GetFunctor() const
{
  return this->Functor;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::SetFunctor(const FunctorT& f)
{
  this->Functor = f;
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
double vtkBinsAccumulator<FunctorT>::GetValue() const
{
  return this->Value;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
bool vtkBinsAccumulator<FunctorT>::HasSameParameters(vtkAbstractAccumulator* accumulator) const
{
  vtkBinsAccumulator* acc = vtkBinsAccumulator::SafeDownCast(accumulator);
  return acc && acc->DiscretizationStep == this->DiscretizationStep &&
    this->Functor == acc->GetFunctor();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::ShallowCopy(vtkDataObject* accumulator)
{
  this->Superclass::ShallowCopy(accumulator);
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  if (binsAccumulator)
  {
    this->Bins = binsAccumulator->GetBins();
    this->Functor = binsAccumulator->GetFunctor();
    this->DiscretizationStep = binsAccumulator->GetDiscretizationStep();
  }
  else
  {
    this->Bins = nullptr;
  }
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkBinsAccumulator<FunctorT>::DeepCopy(vtkDataObject* accumulator)
{
  this->Superclass::DeepCopy(accumulator);
  vtkBinsAccumulator* binsAccumulator = vtkBinsAccumulator::SafeDownCast(accumulator);
  if (binsAccumulator)
  {
    const BinsPointer& bins = binsAccumulator->GetBins();
    this->Bins = std::make_shared<BinsType>(bins->cbegin(), bins->cend());
    this->Functor = binsAccumulator->GetFunctor();
    this->DiscretizationStep = binsAccumulator->GetDiscretizationStep();
  }
  else
  {
    this->Bins = nullptr;
  }
}

#endif
