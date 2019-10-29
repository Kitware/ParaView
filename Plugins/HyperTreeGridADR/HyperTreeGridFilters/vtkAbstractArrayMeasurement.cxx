/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArrayMeasurement.h"

#include "vtkAbstractAccumulator.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkAbstractObjectFactoryNewMacro(vtkAbstractArrayMeasurement);

//----------------------------------------------------------------------------
vtkAbstractArrayMeasurement::vtkAbstractArrayMeasurement()
  : NumberOfAccumulatedData(0)
  , TotalWeight(0.0)
{
}

//----------------------------------------------------------------------------
vtkAbstractArrayMeasurement::~vtkAbstractArrayMeasurement()
{
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    if (this->Accumulators[i])
    {
      this->Accumulators[i]->Delete();
      this->Accumulators[i] = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(double* data, vtkIdType numberOfComponents, double weight)
{
  assert(this->Accumulators.size() && "Accumulators are not allocated");
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(data, numberOfComponents, weight);
  }
  this->TotalWeight += weight;
  ++(this->NumberOfAccumulatedData);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(vtkDataArray* data, vtkDoubleArray* weights)
{
  assert((weights == nullptr || data->GetNumberOfTuples() <= weights->GetNumberOfTuples()) &&
    "data and weights do not have same number of tuples");
  assert(this->Accumulators.size() && "Accumulators are not allocated");
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(data, weights);
  }
  this->NumberOfAccumulatedData += data->GetNumberOfTuples();
  for (vtkIdType i = 0; i < this->NumberOfAccumulatedData; ++i)
  {
    this->TotalWeight += weights ? weights->GetTuple1(i) : 1.0;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(vtkAbstractArrayMeasurement* arrayMeasurement)
{
  assert(this->Accumulators.size() && "Accumulators are not allocated");
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(arrayMeasurement->GetAccumulators()[i]);
  }
  this->TotalWeight += arrayMeasurement->GetTotalWeight();
  this->NumberOfAccumulatedData += arrayMeasurement->GetNumberOfAccumulatedData();
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkAbstractArrayMeasurement::Measure(double& value)
{
  return this->Measure(
    this->Accumulators.data(), this->NumberOfAccumulatedData, this->TotalWeight, value);
}

//----------------------------------------------------------------------------
bool vtkAbstractArrayMeasurement::CanMeasure() const
{
  return this->CanMeasure(this->NumberOfAccumulatedData, this->TotalWeight);
}

//----------------------------------------------------------------------------
std::vector<vtkAbstractAccumulator*>& vtkAbstractArrayMeasurement::GetAccumulators()
{
  return this->Accumulators;
}

//----------------------------------------------------------------------------
const std::vector<vtkAbstractAccumulator*>& vtkAbstractArrayMeasurement::GetAccumulators() const
{
  return this->Accumulators;
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Initialize()
{
  this->NumberOfAccumulatedData = 0;
  this->TotalWeight = 0.0;
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Delete();
    this->Accumulators[i] = nullptr;
  }
  this->Accumulators = this->NewAccumulatorInstances();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::ShallowCopy(vtkDataObject* o)
{
  this->Superclass::ShallowCopy(o);
  vtkAbstractArrayMeasurement* arrayMeasurement = vtkAbstractArrayMeasurement::SafeDownCast(o);
  if (arrayMeasurement &&
    this->GetNumberOfAccumulators() == arrayMeasurement->GetNumberOfAccumulators())
  {
    auto& accumulators = arrayMeasurement->GetAccumulators();
    if (!this->Accumulators.size() && accumulators.size())
    {
      this->Accumulators.resize(accumulators.size());
    }
    for (std::size_t i = 0; i < accumulators.size(); ++i)
    {
      this->Accumulators[i]->ShallowCopy(accumulators[i]);
    }
    this->TotalWeight = arrayMeasurement->GetTotalWeight();
    this->NumberOfAccumulatedData = arrayMeasurement->GetNumberOfAccumulatedData();
    this->Modified();
  }
  else
  {
    vtkWarningMacro(<< "Could not copy vtkAbstractArrayMeasurement, not the same number of "
                       "accumulators, or incorrect type");
  }
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::DeepCopy(vtkDataObject* o)
{
  this->Superclass::DeepCopy(o);
  vtkAbstractArrayMeasurement* arrayMeasurement = vtkAbstractArrayMeasurement::SafeDownCast(o);
  if (arrayMeasurement &&
    this->GetNumberOfAccumulators() == arrayMeasurement->GetNumberOfAccumulators())
  {
    auto& accumulators = arrayMeasurement->GetAccumulators();
    if (!this->Accumulators.size() && accumulators.size())
    {
      this->Accumulators.resize(accumulators.size());
    }
    for (std::size_t i = 0; i < accumulators.size(); ++i)
    {
      this->Accumulators[i]->DeepCopy(accumulators[i]);
    }
    this->TotalWeight = arrayMeasurement->GetTotalWeight();
    this->NumberOfAccumulatedData = arrayMeasurement->GetNumberOfAccumulatedData();
    this->Modified();
  }
  else
  {
    vtkWarningMacro(<< "Could not copy vtkAbstractArrayMeasurement, not the same number of "
                       "accumulators, or incorrect type");
  }
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfAccumulatedData : " << this->NumberOfAccumulatedData << std::endl;
  os << indent << "TotalWeight : " << this->TotalWeight << std::endl;
  os << indent << "NumberOfAccumulators : " << this->GetNumberOfAccumulators() << std::endl;
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    os << indent << "Accumulator " << i << ": " << std::endl;
    os << indent << *(this->Accumulators[i]) << std::endl;
  }
}
