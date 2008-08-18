/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataExtractDataSets.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractDataSets.h"

#include "vtkObjectFactory.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkExtractDataSets.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include <vtkstd/set>

class vtkMultiGroupDataExtractDataSets::vtkInternals
{
public:
  struct vtkInfo
    {
    unsigned int GroupNo;
    unsigned int DataSetNo;
    bool operator < (const vtkInfo& b) const
      {
      return (this->GroupNo == b.GroupNo)? (this->GroupNo < b.GroupNo) :
        (this->DataSetNo < b.DataSetNo);
      }
    };
  typedef vtkstd::set<vtkInfo> vtkInfoSet;
  vtkInfoSet DataSets;
};

vtkStandardNewMacro(vtkMultiGroupDataExtractDataSets);
vtkCxxRevisionMacro(vtkMultiGroupDataExtractDataSets, "1.1");
//----------------------------------------------------------------------------
vtkMultiGroupDataExtractDataSets::vtkMultiGroupDataExtractDataSets()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractDataSets::~vtkMultiGroupDataExtractDataSets()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::AddDataSet(unsigned int group,
  unsigned int idx)
{
  vtkInternals::vtkInfo info;
  info.GroupNo = group;
  info.DataSetNo = idx;
  this->Internals->DataSets.insert(info);
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::ClearDataSetList()
{
  this->Internals->DataSets.clear();
}


//----------------------------------------------------------------------------
// Output type is same as input.
int vtkMultiGroupDataExtractDataSets::RequestDataObject(
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
int vtkMultiGroupDataExtractDataSets::RequestData(
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
  
  vtkHierarchicalBoxDataSet* hbInput = vtkHierarchicalBoxDataSet::SafeDownCast(input);
  if (hbInput)
    {
    vtkHierarchicalBoxDataSet* clone = vtkHierarchicalBoxDataSet::New();
    clone->ShallowCopy(hbInput);

    vtkExtractDataSets *ed = vtkExtractDataSets::New();
    vtkInternals::vtkInfoSet::iterator iter;
    for (iter = this->Internals->DataSets.begin(); 
         iter != this->Internals->DataSets.end();
         iter++)
      {
      ed->AddDataSet(iter->GroupNo, iter->DataSetNo);
      }
    ed->SetInput(clone);
    clone->Delete();
    ed->Update();
    output->ShallowCopy(ed->GetOutput());
    ed->Delete();
    return 1;
    }

  vtkMultiBlockDataSet* mbInput = vtkMultiBlockDataSet::SafeDownCast(input);
  if (mbInput)
    {
    vtkErrorMacro("Please take a look at replacing this filter with vtkExtractBlock.");
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


