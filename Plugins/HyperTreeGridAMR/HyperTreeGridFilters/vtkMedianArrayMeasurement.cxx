#include "vtkMedianArrayMeasurement.h"

#include "vtkMedianAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkMedianArrayMeasurement);

//----------------------------------------------------------------------------
vtkMedianArrayMeasurement::vtkMedianArrayMeasurement()
{
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkMedianAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkMedianArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue();
}

//----------------------------------------------------------------------------
void vtkMedianArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
