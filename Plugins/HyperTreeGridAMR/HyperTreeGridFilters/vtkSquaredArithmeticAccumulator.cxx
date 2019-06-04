#include "vtkSquaredArithmeticAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkSquaredArithmeticAccumulator);

//----------------------------------------------------------------------------
vtkSquaredArithmeticAccumulator::vtkSquaredArithmeticAccumulator()
{
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkSquaredArithmeticAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Value += accumulator->GetValue();
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Add(double value)
{
  this->Value += value * value;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 0.0;
}

//----------------------------------------------------------------------------
void vtkSquaredArithmeticAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
