/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArrayMeasurement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
  for (std::size_t i = 0; i < this->Accumulators.size(); ++i)
  {
    if (this->Accumulators[i])
    {
      this->Accumulators[i]->Delete();
      this->Accumulators[i] = nullptr;
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
bool vtkAbstractArrayMeasurement::CanMeasure() const
{
  return this->NumberOfAccumulatedData;
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
