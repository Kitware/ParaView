/*=========================================================================

  Program:   ParaView
  Module:    vtkCPNodalFieldBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPNodalFieldBuilder.h"

#include "vtkCPTensorFieldFunction.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <vector>

vtkStandardNewMacro(vtkCPNodalFieldBuilder);

//----------------------------------------------------------------------------
vtkCPNodalFieldBuilder::vtkCPNodalFieldBuilder() = default;

//----------------------------------------------------------------------------
void vtkCPNodalFieldBuilder::BuildField(unsigned long timeStep, double time, vtkDataSet* grid)
{
  vtkCPTensorFieldFunction* tensorFieldFunction = this->GetTensorFieldFunction();
  if (tensorFieldFunction == nullptr)
  {
    vtkErrorMacro("Must set TensorFieldFunction.");
    return;
  }
  if (this->GetArrayName() == nullptr)
  {
    vtkErrorMacro("Must set ArrayName.");
    return;
  }
  vtkIdType numberOfPoints = grid->GetNumberOfPoints();
  unsigned int numberOfComponents = tensorFieldFunction->GetNumberOfComponents();
  vtkDoubleArray* array = vtkDoubleArray::New();
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfPoints);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3];
  for (vtkIdType i = 0; i < numberOfPoints; i++)
  {
    grid->GetPoint(i, point);
    for (unsigned int uj = 0; uj < numberOfComponents; uj++)
    {
      tupleValues[uj] = tensorFieldFunction->ComputeComponenentAtPoint(uj, point, timeStep, time);
    }
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  array->SetName(this->GetArrayName());
  grid->GetPointData()->AddArray(array);
  array->Delete();
}

//----------------------------------------------------------------------------
vtkCPNodalFieldBuilder::~vtkCPNodalFieldBuilder() = default;

//----------------------------------------------------------------------------
void vtkCPNodalFieldBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
