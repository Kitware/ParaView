/*=========================================================================

  Program:   ParaView
  Module:    vtkMergeArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeArrays.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkMergeArrays);

//----------------------------------------------------------------------------
vtkMergeArrays::vtkMergeArrays()
{
}

//----------------------------------------------------------------------------
vtkMergeArrays::~vtkMergeArrays()
{
}

//----------------------------------------------------------------------------
int vtkMergeArrays::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkMergeArrays::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int idx;
  int numCells, numPoints;
  int numArrays, arrayIdx;
  vtkDataSet* input;
  vtkDataArray* array;
  int num = inputVector[0]->GetNumberOfInformationObjects();
  if (num < 1)
  {
    return 0;
  }

  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  numCells = input->GetNumberOfCells();
  numPoints = input->GetNumberOfPoints();
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  for (idx = 1; idx < num; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (output->GetNumberOfPoints() == numPoints && output->GetNumberOfCells() == numCells)
    {
      numArrays = input->GetPointData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        array = input->GetPointData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetPointData()->AddArray(array);
      }
      numArrays = input->GetCellData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        array = input->GetCellData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetCellData()->AddArray(array);
      }
      numArrays = input->GetFieldData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        array = input->GetFieldData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetFieldData()->AddArray(array);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMergeArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
