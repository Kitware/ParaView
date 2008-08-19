/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataGroupIdScalars.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataGroupIdScalars.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkUnsignedCharArray.h"
vtkStandardNewMacro(vtkMultiGroupDataGroupIdScalars);
vtkCxxRevisionMacro(vtkMultiGroupDataGroupIdScalars, "1.2");
//----------------------------------------------------------------------------
vtkMultiGroupDataGroupIdScalars::vtkMultiGroupDataGroupIdScalars()
{
}

//----------------------------------------------------------------------------
vtkMultiGroupDataGroupIdScalars::~vtkMultiGroupDataGroupIdScalars()
{
}

//----------------------------------------------------------------------------
// Output type is same as input.
int vtkMultiGroupDataGroupIdScalars::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
    {
    vtkDataObject *output = vtkDataObject::GetData(outInfo);
    if (!output || output->IsA(input->GetClassName()) == 0)
      {
      vtkDataObject* newOutput = input->NewInstance();
      newOutput->SetPipelineInformation(outInfo);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataGroupIdScalars::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input) 
    {
    return 0;
    }

  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector, 0);
  if (!output) 
    {
    return 0;
    }

  output->ShallowCopy(input);
  return this->ColorBlock(output, 0);
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataGroupIdScalars::ColorBlock(vtkDataObject* dObj, int group_no)
{
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(dObj);
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    iter->VisitOnlyLeavesOff();
    iter->TraverseSubTreeOff();
    iter->SkipEmptyNodesOff();

    int new_group_no = 0;
    for (iter->InitTraversal();
      !iter->IsDoneWithTraversal();
      iter->GoToNextItem(), new_group_no++)
      {
      vtkDataObject* child = iter->GetCurrentDataObject();
      if (child)
        {
        this->ColorBlock(child, new_group_no);
        }
      }
    iter->Delete();
    }
  else if (ds)
    {
    vtkIdType numCells = ds->GetNumberOfCells();
    vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
    cArray->SetNumberOfTuples(numCells);
    cArray->FillComponent(0, group_no);
    cArray->SetName("GroupIdScalars");
    ds->GetCellData()->AddArray(cArray);
    cArray->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataGroupIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


