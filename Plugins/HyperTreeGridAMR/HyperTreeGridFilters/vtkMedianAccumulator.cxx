#include "vtkMedianAccumulator.h"

#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkMedianAccumulator);

//----------------------------------------------------------------------------
void vtkMedianAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(
    vtkMedianAccumulator::SafeDownCast(accumulator) && "Cannot accumulate different accumulators");
  vtkMedianAccumulator* medianAccumulator = vtkMedianAccumulator::SafeDownCast(accumulator);
  std::vector<double> out;
  std::merge(this->SortedList.begin(), this->SortedList.end(),
    medianAccumulator->GetSortedList().cbegin(), medianAccumulator->GetSortedList().cend(),
    std::back_inserter(out));
  this->SortedList = std::move(out);
}

//----------------------------------------------------------------------------
double vtkMedianAccumulator::GetValue() const
{
  return this->SortedList[this->SortedList.size() / 2];
}

//----------------------------------------------------------------------------
void vtkMedianAccumulator::Add(double value)
{
  auto it = std::lower_bound(this->SortedList.begin(), this->SortedList.end(), value);
  this->SortedList.insert(it, value);
}

//----------------------------------------------------------------------------
void vtkMedianAccumulator::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkMedianAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->GetValue() << std::endl;
  os << indent << "Sorted list: [";
  for (std::size_t i = 0; i < this->SortedList.size(); ++i)
  {
    os << indent << this->SortedList[i] << ", ";
  }
  os << indent << "]" << std::endl;
}
