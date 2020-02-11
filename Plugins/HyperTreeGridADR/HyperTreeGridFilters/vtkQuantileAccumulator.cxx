/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantileAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuantileAccumulator.h"

#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <vector>

vtkStandardNewMacro(vtkQuantileAccumulator);

//----------------------------------------------------------------------------
vtkQuantileAccumulator::vtkQuantileAccumulator()
  : PercentileIdx(0)
  , Percentile(50.0)
  , TotalWeight(0.0)
  , PercentileWeight(0.0)
  , SortedList(std::make_shared<ListType>())
{
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  vtkQuantileAccumulator* quantileAccumulator = vtkQuantileAccumulator::SafeDownCast(accumulator);
  assert(quantileAccumulator && "Cannot accumulate different accumulators");

  if (this->SortedList->size())
  {
    std::size_t i = 0;
    ListType out;
    while (i < quantileAccumulator->SortedList->size() &&
      (*quantileAccumulator->SortedList)[i].Value < (*this->SortedList)[this->PercentileIdx].Value)
    {
      this->PercentileWeight += (*quantileAccumulator->SortedList)[i++].Weight;
    }
    this->PercentileIdx += i;
    std::merge(this->SortedList->begin(), this->SortedList->end(),
      quantileAccumulator->SortedList->cbegin(), quantileAccumulator->SortedList->cend(),
      std::back_inserter(out));
    this->SortedList = std::make_shared<ListType>(out);
    this->TotalWeight += quantileAccumulator->TotalWeight;

    // Move the percentile in the left direction.
    while (this->PercentileIdx != 0 &&
      this->Percentile - 100.0 * this->PercentileWeight / this->TotalWeight <= 0)
    {
      this->PercentileWeight -= (*this->SortedList)[this->PercentileIdx].Weight;
      --this->PercentileIdx;
    }
    // Move the percentile in the right direction.
    while (this->PercentileIdx != this->SortedList->size() - 1 &&
      this->Percentile - 100.0 * this->PercentileWeight / this->TotalWeight > 0)
    {
      ++this->PercentileIdx;
      this->PercentileWeight += (*this->SortedList)[this->PercentileIdx].Weight;
    }
  }
  else
  {
    if (quantileAccumulator->SortedList->size())
    {
      this->TotalWeight = quantileAccumulator->TotalWeight;
      this->PercentileIdx = quantileAccumulator->PercentileIdx;
      this->PercentileWeight = quantileAccumulator->PercentileWeight;
      *this->SortedList = *quantileAccumulator->SortedList;
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::Add(double value, double weight)
{
  if (this->SortedList->size() && (*this->SortedList)[this->PercentileIdx].Value >= value)
  {
    this->PercentileWeight += weight;
    ++this->PercentileIdx;
  }
  else if (!this->SortedList->size())
  {
    this->PercentileWeight = weight;
  }
  this->TotalWeight += weight;
  auto it = std::lower_bound(
    this->SortedList->begin(), this->SortedList->end(), ListElement(value, weight));
  this->SortedList->insert(it, ListElement(value, weight));

  // Move the percentile in the left direction.
  while (this->PercentileIdx != 0 &&
    this->Percentile - 100.0 * this->PercentileWeight / this->TotalWeight <= 0)
  {
    this->PercentileWeight -= (*this->SortedList)[this->PercentileIdx].Weight;
    --this->PercentileIdx;
  }
  // Move the percentile in the right direction.
  while (this->PercentileIdx != this->SortedList->size() - 1 &&
    this->Percentile - 100.0 * this->PercentileWeight / this->TotalWeight > 0)
  {
    ++this->PercentileIdx;
    this->PercentileWeight += (*this->SortedList)[this->PercentileIdx].Weight;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::Initialize()
{
  this->SortedList->clear();
  this->PercentileIdx = 0;
  this->TotalWeight = 0.0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PercentileIdx " << this->PercentileIdx << std::endl;
  os << indent << "PercentileWeight " << this->PercentileWeight << std::endl;
  os << indent << "TotalWeight " << this->TotalWeight << std::endl;
  os << indent << "Sorted list:" << std::endl;
  for (std::size_t i = 0; i < this->SortedList->size(); ++i)
  {
    os << indent << indent << "Index " << i << ": (Value: " << (*this->SortedList)[i].Value
       << ", Weight: " << (*this->SortedList)[i].Weight << ")" << std::endl;
  }
}

//----------------------------------------------------------------------------
const std::shared_ptr<vtkQuantileAccumulator::ListType>& vtkQuantileAccumulator::GetSortedList()
  const
{
  return this->SortedList;
}

//----------------------------------------------------------------------------
bool vtkQuantileAccumulator::HasSameParameters(vtkAbstractAccumulator* accumulator) const
{
  vtkQuantileAccumulator* quantileAccumulator = vtkQuantileAccumulator::SafeDownCast(accumulator);
  return quantileAccumulator != nullptr && this->Percentile == quantileAccumulator->GetPercentile();
}

//----------------------------------------------------------------------------
double vtkQuantileAccumulator::GetValue() const
{
  return this->SortedList->size() ? (*this->SortedList)[this->PercentileIdx].Value : 0.0;
}

//----------------------------------------------------------------------------
bool vtkQuantileAccumulator::ListElement::operator<(const ListElement& el) const
{
  return this->Value < el.Value;
}

//----------------------------------------------------------------------------
vtkQuantileAccumulator::ListElement::ListElement(double value, double weight)
  : Value(value)
  , Weight(weight)
{
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::ShallowCopy(vtkDataObject* accumulator)
{
  this->Superclass::ShallowCopy(accumulator);
  vtkQuantileAccumulator* quantileAccumulator = vtkQuantileAccumulator::SafeDownCast(accumulator);
  if (quantileAccumulator)
  {
    this->SortedList = quantileAccumulator->GetSortedList();
    this->SetPercentile(quantileAccumulator->GetPercentile());
  }
  else
  {
    this->SortedList = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::DeepCopy(vtkDataObject* accumulator)
{
  this->Superclass::DeepCopy(accumulator);
  vtkQuantileAccumulator* quantileAccumulator = vtkQuantileAccumulator::SafeDownCast(accumulator);
  if (quantileAccumulator)
  {
    const ListPointer& sortedList = quantileAccumulator->GetSortedList();
    this->SortedList = std::make_shared<ListType>(sortedList->cbegin(), sortedList->cend());
    this->SetPercentile(quantileAccumulator->GetPercentile());
  }
  else
  {
    this->SortedList = nullptr;
  }
}
