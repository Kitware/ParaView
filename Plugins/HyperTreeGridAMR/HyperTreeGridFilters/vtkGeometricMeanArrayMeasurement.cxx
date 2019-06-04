#include "vtkGeometricMeanArrayMeasurement.h"

#include "vtkGeometricAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkGeometricMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkGeometricMeanArrayMeasurement::vtkGeometricMeanArrayMeasurement()
{
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkGeometricAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkGeometricMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->Accumulators[0]->GetValue() / this->NumberOfAccumulatedData;
}

//----------------------------------------------------------------------------
void vtkGeometricMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
