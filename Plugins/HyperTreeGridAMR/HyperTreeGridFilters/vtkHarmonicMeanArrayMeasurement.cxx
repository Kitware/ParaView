#include "vtkHarmonicMeanArrayMeasurement.h"

#include "vtkInversedArithmeticAccumulator.h"

#include <cassert>

vtkStandardNewMacro(vtkHarmonicMeanArrayMeasurement);

//----------------------------------------------------------------------------
vtkHarmonicMeanArrayMeasurement::vtkHarmonicMeanArrayMeasurement()
{
  this->Accumulators.resize(1);
  this->Accumulators[0] = vtkInversedArithmeticAccumulator::New();
}

//----------------------------------------------------------------------------
double vtkHarmonicMeanArrayMeasurement::Measure() const
{
  assert(this->Accumulators.size() && "No accumulator, cannot measure");
  return this->NumberOfAccumulatedData / this->Accumulators[0]->GetValue();
}

//----------------------------------------------------------------------------
void vtkHarmonicMeanArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
