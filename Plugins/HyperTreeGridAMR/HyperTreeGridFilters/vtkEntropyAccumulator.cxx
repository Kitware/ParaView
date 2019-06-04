#include "vtkEntropyAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkEntropyAccumulator);

//----------------------------------------------------------------------------
vtkEntropyAccumulator::vtkEntropyAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkEntropyAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(
    vtkEntropyAccumulator::SafeDownCast(accumulator) && "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkEntropyAccumulator::Add(double value)
{
  this->Value += value * std::log(value);
}

//----------------------------------------------------------------------------
void vtkEntropyAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkEntropyAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
