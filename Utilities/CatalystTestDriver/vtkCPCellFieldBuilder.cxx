/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCellFieldBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPCellFieldBuilder.h"

#include "vtkCPTensorFieldFunction.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"

#include <vector>

vtkStandardNewMacro(vtkCPCellFieldBuilder);

//----------------------------------------------------------------------------
vtkCPCellFieldBuilder::vtkCPCellFieldBuilder()
{
}

//----------------------------------------------------------------------------
void vtkCPCellFieldBuilder::BuildField(unsigned long timeStep, double time, vtkDataSet* grid)
{
  vtkCPTensorFieldFunction* tensorFieldFunction = this->GetTensorFieldFunction();
  if (tensorFieldFunction == 0)
  {
    vtkErrorMacro("Must set TensorFieldFunction.");
    return;
  }
  if (this->GetArrayName() == 0)
  {
    vtkErrorMacro("Must set ArrayName.");
    return;
  }
  vtkIdType numberOfCells = grid->GetNumberOfCells();
  unsigned int numberOfComponents = tensorFieldFunction->GetNumberOfComponents();
  vtkDoubleArray* array = vtkDoubleArray::New();
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfCells);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3], parametricCenter[3], weights[100];
  for (vtkIdType i = 0; i < numberOfCells; i++)
  {
    vtkCell* cell = grid->GetCell(i);
    cell->GetParametricCenter(parametricCenter);
    int subId = 0;
    cell->EvaluateLocation(subId, parametricCenter, point, weights);
    for (unsigned int uj = 0; uj < numberOfComponents; uj++)
    {
      tupleValues[uj] = tensorFieldFunction->ComputeComponenentAtPoint(uj, point, timeStep, time);
    }
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  grid->GetCellData()->AddArray(array);
  array->Delete();
}

//----------------------------------------------------------------------------
vtkCPCellFieldBuilder::~vtkCPCellFieldBuilder()
{
}

//----------------------------------------------------------------------------
void vtkCPCellFieldBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
