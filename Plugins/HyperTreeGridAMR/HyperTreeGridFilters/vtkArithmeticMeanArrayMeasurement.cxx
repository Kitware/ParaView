#include "vtkArithmeticMeanArrayMeasurement.h"

#include "vtkArithmeticAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkArithmeticMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkArithmeticMeanArrayMeasurement::vtkArithmeticMeanArrayMeasurement()
{
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkArithmeticAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkArithmeticMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue() / this->NumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
void vtkArithmeticMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
