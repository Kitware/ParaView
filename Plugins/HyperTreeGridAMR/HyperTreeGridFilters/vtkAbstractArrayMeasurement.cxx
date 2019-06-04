#include "vtkAbstractArrayMeasurement.h"

#include "vtkAbstractAccumulator.h"

//----------------------------------------------------------------------------
vtkAbstractArrayMeasurement::vtkAbstractArrayMeasurement()
{
  this->NumberOfAccumulatedData = 0;
}

//----------------------------------------------------------------------------
vtkAbstractArrayMeasurement::~vtkAbstractArrayMeasurement()
{
  if (this->Accumulators.size())
  {
    if (this->Accumulators[0])
    {
      this->Accumulators[0]->Delete();
      this->Accumulators[0] = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(double* data, vtkIdType numberOfComponents)
{
  for (vtkIdType i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(data, numberOfComponents);
  }
  this->NumberOfAccumulatedData += 1;
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(vtkDataArray* data)
{
  for (vtkIdType i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(data);
  }
  this->NumberOfAccumulatedData += data->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Add(vtkAbstractArrayMeasurement* arrayMeasurement)
{
  for (vtkIdType i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Add(arrayMeasurement->GetAccumulators()[i]);
  }
  this->NumberOfAccumulatedData += arrayMeasurement->GetNumberOfAccumulatedData();
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::Initialize()
{
  this->NumberOfAccumulatedData = 0;
  for (vtkIdType i = 0; i < this->Accumulators.size(); ++i)
  {
    this->Accumulators[i]->Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractArrayMeasurement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfAccumulatedData : " << this->NumberOfAccumulatedData << std::endl;
  os << indent << "NumberOfAccumulators : " << this->Accumulators.size() << std::endl;
  for (vtkIdType i = 0; i < this->Accumulators.size(); ++i)
  {
    os << indent << "Accumulator " << i << ": " << std::endl;
    os << indent << *(this->Accumulators[i]) << std::endl;
  }
}
