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

  ListType out;
  std::merge(this->SortedList->begin(), this->SortedList->end(),
    quantileAccumulator->GetSortedList()->cbegin(), quantileAccumulator->GetSortedList()->cend(),
    std::back_inserter(out));
  this->SortedList = std::make_shared<ListType>(out);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuantileAccumulator::Add(double value, double weight)
{
  this->TotalWeight += weight;
  auto it = std::lower_bound(
    this->SortedList->begin(), this->SortedList->end(), ListElement(value, weight));
  this->SortedList->insert(it, ListElement(value, weight));
  if ((*this->SortedList)[this->PercentileIdx].Value >= value)
  {
    this->PercentileWeight += weight;

    // Move the percentile in the left direction accordingly.
    while (this->PercentileIdx != 0 &&
      this->Percentile -
          100.0 * (this->PercentileWeight - (*this->SortedList)[this->PercentileIdx - 1].Weight) /
            this->TotalWeight >=
        0)
    {
      --this->PercentileIdx;
      this->PercentileWeight -= (*this->SortedList)[this->PercentileIdx].Weight;
    }
  }
  else
  {
    while (this->PercentileIdx < this->SortedList->size() &&
      this->Percentile -
          100.0 * (this->PercentileWeight + (*this->SortedList)[this->PercentileIdx + 1].Weight) /
            this->TotalWeight <=
        0)
    {
      ++this->PercentileIdx;
      this->PercentileWeight += (*this->SortedList)[this->PercentileIdx].Weight;
    }
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
  os << indent << "Sorted list:" << std::endl;
  for (std::size_t i = 0; i < this->SortedList->size(); ++i)
  {
    os << indent << "Index " << i << ": (Value: " << (*this->SortedList)[i].Value
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
  return (*this->SortedList)[this->PercentileIdx].Value;
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
  }
  else
  {
    this->SortedList = nullptr;
  }
}
