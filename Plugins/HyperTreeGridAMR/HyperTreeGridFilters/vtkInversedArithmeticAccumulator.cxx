#include "vtkInversedArithmeticAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkInversedArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkInversedArithmeticAccumulator::vtkInversedArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkInversedArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Add(double value)
{
  assert(value != 0 && "Cannot add null values into inversed arithmetic accumulator");
  this->Value += 1 / value;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkInversedArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
