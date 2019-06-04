#include "vtkArithmeticAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkArithmeticAccumulator::vtkArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Add(accumulator->GetValue());
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Add(double value)
{
  this->Value += value;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
