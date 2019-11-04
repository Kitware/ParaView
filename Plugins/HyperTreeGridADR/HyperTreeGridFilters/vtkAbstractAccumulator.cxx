/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractAccumulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractAccumulator.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"

#include <cmath>

vtkAbstractObjectFactoryNewMacro(vtkAbstractAccumulator);

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
void vtkAbstractAccumulator::Add(vtkDataArray* data, vtkDoubleArray* weights)
{
  for (vtkIdType i = 0; i < data->GetNumberOfTuples(); ++i)
  {
    if (data->GetNumberOfComponents() > 1)
    {
      this->Add(this->ConvertVectorToScalar(data->GetTuple(i), data->GetNumberOfComponents()),
        weights->GetTuple1(i));
    }
    else
    {
      this->Add(data->GetTuple1(i), weights->GetTuple1(i));
    }
  }
}

//----------------------------------------------------------------------------
void vtkAbstractAccumulator::Add(const double* data, vtkIdType numberOfComponents, double weight)
{
  if (numberOfComponents > 1)
  {
    this->Add(this->ConvertVectorToScalar(data, numberOfComponents), weight);
  }
  else
  {
    this->Add(*data, weight);
  }
}

//----------------------------------------------------------------------------
void vtkAbstractAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Value: " << this->GetValue() << std::endl;
}
