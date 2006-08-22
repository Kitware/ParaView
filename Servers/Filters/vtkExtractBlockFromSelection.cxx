/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractBlockFromSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractBlockFromSelection.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkExtractBlockFromSelection, "1.3");
vtkStandardNewMacro(vtkExtractBlockFromSelection);

//----------------------------------------------------------------------------
vtkExtractBlockFromSelection::vtkExtractBlockFromSelection()
{
  this->SourceID = 0;
} 

//----------------------------------------------------------------------------
vtkExtractBlockFromSelection::~vtkExtractBlockFromSelection()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkExtractBlockFromSelection::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkExtractBlockFromSelection::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet *mbInput = vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCompositeDataIterator* iter = mbInput->NewIterator();
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkUnstructuredGrid* ug = 
      vtkUnstructuredGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (ug)
      {
      if (ug->GetInformation()->Get(vtkSelection::SOURCE_ID()) ==
          this->SourceID)
        {
        output->ShallowCopy(ug);
        }
      iter->Delete();
      return 1;
      }
    }
  iter->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractBlockFromSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkExtractBlockFromSelection::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkMultiBlockDataSet");

  return 1;
}
