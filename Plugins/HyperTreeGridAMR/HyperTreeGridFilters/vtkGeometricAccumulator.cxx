#include "vtkGeometricAccumulator.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkGeometricAccumulator);

//----------------------------------------------------------------------------
vtkGeometricAccumulator::vtkGeometricAccumulator()
{
  this->Value = 1.0;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Add(vtkAbstractAccumulator* accumulator)
{
  assert(vtkGeometricAccumulator::SafeDownCast(accumulator) &&
    "Cannot accumulate different accumulators");
  this->Add(accumulator->GetValue());
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Add(double value)
{
  assert(value > 0 && "Cannot add null or negative values into a geometric accumulator");
  this->Value *= value;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::Initialize()
{
  this->Superclass::Initialize();
  this->Value = 1.0;
}

//----------------------------------------------------------------------------
void vtkGeometricAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Accumulated value : " << this->Value << std::endl;
}
