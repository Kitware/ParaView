/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticAccumulator.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkArithmeticAccumulator_txx
#define vtkArithmeticAccumulator_txx

#include "vtkArithmeticAccumulator.h"

#include "vtkFunctor.h"
#include "vtkObjectFactory.h"

#include <functional>

template <typename FunctorT>
vtkStandardNewMacro(vtkArithmeticAccumulator<FunctorT>);

//----------------------------------------------------------------------------
template <typename FunctorT>
vtkArithmeticAccumulator<FunctorT>::vtkArithmeticAccumulator()
  : Value(0.0)
{
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::Add(vtkAbstractAccumulator* accumulator)
{
  vtkArithmeticAccumulator<FunctorT>* arithmeticAccumulator =
    vtkArithmeticAccumulator<FunctorT>::SafeDownCast(accumulator);
  assert(arithmeticAccumulator && "Cannot accumulate different accumulators");
  this->Value += arithmeticAccumulator->GetValue();
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::Add(double value, double weight)
{
  this->Value += weight * this->Functor(value);
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::Initialize()
{
  this->Value = 0.0;
  this->Functor = FunctorType();
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
double vtkArithmeticAccumulator<FunctorT>::GetValue() const
{
  return this->Value;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
const FunctorT& vtkArithmeticAccumulator<FunctorT>::GetFunctor() const
{
  return this->Functor;
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::SetFunctor(const FunctorT&& f)
{
  this->Functor = f;
  this->Modified();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
bool vtkArithmeticAccumulator<FunctorT>::HasSameParameters(
  vtkAbstractAccumulator* accumulator) const
{
  vtkArithmeticAccumulator<FunctorT>* acc = vtkArithmeticAccumulator::SafeDownCast(accumulator);
  // We compare the pointer on the function used.
  return acc && this->Functor == acc->GetFunctor();
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::ShallowCopy(vtkDataObject* accumulator)
{
  this->Superclass::ShallowCopy(accumulator);
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulator);
  if (arithmeticAccumulator)
  {
    this->Value = arithmeticAccumulator->GetValue();
    this->Functor = arithmeticAccumulator->GetFunctor();
  }
  else
  {
    vtkWarningMacro(<< "Could not ShallowCopy " << arithmeticAccumulator->GetClassName() << " to "
                    << this->GetClassName());
  }
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::DeepCopy(vtkDataObject* accumulator)
{
  this->Superclass::DeepCopy(accumulator);
  vtkArithmeticAccumulator* arithmeticAccumulator =
    vtkArithmeticAccumulator::SafeDownCast(accumulator);
  if (arithmeticAccumulator)
  {
    this->Value = arithmeticAccumulator->GetValue();
    this->Functor = arithmeticAccumulator->GetFunctor();
  }
  else
  {
    vtkWarningMacro(<< "Could not DeepCopy " << arithmeticAccumulator->GetClassName() << " to "
                    << this->GetClassName());
  }
}

//----------------------------------------------------------------------------
template <typename FunctorT>
void vtkArithmeticAccumulator<FunctorT>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Functor: " << typeid(this->Functor).name() << std::endl;
}

#endif
