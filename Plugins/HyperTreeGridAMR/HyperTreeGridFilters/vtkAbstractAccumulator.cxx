#include "vtkAbstractAccumulator.h"

#include <cmath>

//----------------------------------------------------------------------------
vtkAbstractAccumulator::vtkAbstractAccumulator()
{
  this->ConvertVectorToScalar = [](const double* data, vtkIdType numberOfComponents) {
    double value = 0;
    for (vtkIdType i = 0; i < numberOfComponents; ++i)
    {
      value += data[i] * data[i];
    }
    return std::sqrt(value);
  };
}

//----------------------------------------------------------------------------
void vtkAbstractAccumulator::Add(vtkDataArray* data)
{
  for (vtkIdType i = 0; i < data->GetNumberOfTuples(); ++i)
  {
    if (data->GetNumberOfComponents() > 1)
    {
      this->Add(this->ConvertVectorToScalar(data->GetTuple(i), data->GetNumberOfComponents()));
    }
    else
    {
      this->Add(data->GetTuple(i)[0]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkAbstractAccumulator::Add(const double* data, vtkIdType numberOfComponents)
{
  if (numberOfComponents > 1)
  {
    this->Add(this->ConvertVectorToScalar(data, numberOfComponents));
  }
  else
  {
    this->Add(*data);
  }
}

//----------------------------------------------------------------------------
void vtkAbstractAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
