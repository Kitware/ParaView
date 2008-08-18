/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataExtractGroup.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractGroup.h"

#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkExtractLevel.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkMultiGroupDataExtractGroup);
vtkCxxRevisionMacro(vtkMultiGroupDataExtractGroup, "1.2");
//----------------------------------------------------------------------------
vtkMultiGroupDataExtractGroup::vtkMultiGroupDataExtractGroup()
{
  this->MinGroup = 0;
  this->MaxGroup = 0;

}

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractGroup::~vtkMultiGroupDataExtractGroup()
{
}

//----------------------------------------------------------------------------
// Output type is same as input.
int vtkMultiGroupDataExtractGroup::RequestDataObject(
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
int vtkMultiGroupDataExtractGroup::RequestData(
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

  vtkMultiBlockDataSet* mbInput = vtkMultiBlockDataSet::SafeDownCast(input);
  vtkHierarchicalBoxDataSet* hbInput = vtkHierarchicalBoxDataSet::SafeDownCast(input);

  unsigned int numGroups = this->MaxGroup-this->MinGroup+1;

  if (mbInput)
    {
    if (numGroups == 1)
      {
      // Detect the case where we are extracting 1 group and that group
      // is a multigroup dataset. In that situation, directly copy
      // that data object to the output instead of assigning it as
      // as sub-dataset. This is done to avoid creating a multi-group
      // of multi-group when it is not necessary.
      vtkDataObject* block = mbInput->GetBlock(this->MinGroup);
      if (block && block->IsA("vtkMultiBlockDataSet"))
        {
        output->ShallowCopy(block);
        return 1;
        }
      }

    vtkMultiBlockDataSet* mbOutput = vtkMultiBlockDataSet::SafeDownCast(output);
    mbOutput->SetNumberOfBlocks(numGroups);
    for (unsigned int cc=this->MinGroup; cc <= this->MaxGroup; cc++)
      {
      vtkDataObject *inBlock = mbInput->GetBlock(cc);
      if (inBlock)
        {
        vtkDataObject* clone = inBlock->NewInstance();
        clone->ShallowCopy(inBlock);
        mbOutput->SetBlock(cc-this->MinGroup, clone);
        clone->Delete();
        if (mbInput->HasMetaData(cc))
          {
          mbOutput->GetMetaData(cc-this->MinGroup)->Copy(
            mbInput->GetMetaData(cc), /*deep=*/0);
          }
        }
      }

    return 1;
    }

  if (hbInput)
    {
    vtkHierarchicalBoxDataSet* clone = vtkHierarchicalBoxDataSet::New();
    clone->ShallowCopy(hbInput);

    vtkExtractLevel* el = vtkExtractLevel::New();
    for (unsigned int cc=this->MinGroup; cc<=this->MaxGroup;cc++)
      {
      el->AddLevel(cc);
      }
    el->SetInput(clone);
    clone->Delete();
    el->Update();
    output->ShallowCopy(el->GetOutput());
    el->Delete();
    return 1;
    }

  vtkErrorMacro("Unhandled input type: " << input->GetClassName());
  return 0;
}


//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MinGroup:" << this->MinGroup << endl;
  os << indent << "MaxGroup:" << this->MaxGroup << endl;
}


