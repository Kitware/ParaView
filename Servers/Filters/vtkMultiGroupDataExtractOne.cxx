/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMultiGroupDataExtractOne.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractOne.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkMultiGroupDataExtractOne, "1.1.2.1");
vtkStandardNewMacro(vtkMultiGroupDataExtractOne);

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractOne::vtkMultiGroupDataExtractOne()
{
}

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractOne::~vtkMultiGroupDataExtractOne()
{
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractOne::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkMultiGroupDataSet *input = 
    vtkMultiGroupDataSet::GetData(inputVector[0], 0);
  vtkDataSet *output = vtkDataSet::GetData(outputVector, 0);
  
  if (input)
    {
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->InitTraversal();
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      output = ds->NewInstance();
      output->SetPipelineInformation(outputVector->GetInformationObject(0));
      output->Delete();
      iter->Delete();
      return 1;
      }
    iter->Delete();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractOne::RequestInformation (
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *input = 
    vtkMultiGroupDataSet::GetData(inputVector[0], 0);

  if (input)
    {
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->InitTraversal();
    vtkImageData* id = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
    if (id)
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   id->GetExtent(),
                   6);
      }
    iter->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractOne::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int* ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  cout << "Update extent: "
       << ext[0] << " "
       << ext[1] << " "
       << ext[2] << " "
       << ext[3] << " "
       << ext[4] << " "
       << ext[5] << " "
       << endl;

  vtkMultiGroupDataSet *input = 
    vtkMultiGroupDataSet::GetData(inputVector[0], 0);
  vtkDataSet *output = vtkDataSet::GetData(outputVector, 0);

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->InitTraversal();
  vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
  if (ds)
    {
    output->ShallowCopy(ds);
    iter->Delete();
    return 1;
    }

  iter->Delete();
  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractOne::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiGroupDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkMultiGroupDataExtractOne::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractOne::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
