/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipDataSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClipDataSet.h"

#include "vtkAMRDualClip.h"
#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtkInformationStringVectorKey.h"

#include <cassert>

vtkStandardNewMacro(vtkPVClipDataSet);

//----------------------------------------------------------------------------
vtkPVClipDataSet::vtkPVClipDataSet(vtkImplicitFunction *vtkNotUsed(cf))
{
  // setting NumberOfOutputPorts to 1 because ParaView does not allow you to
  // generate the clipped output
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVClipDataSet::~vtkPVClipDataSet()
{
}

//----------------------------------------------------------------------------
void vtkPVClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestData(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  vtkInformation * inInfo = inputVector[0]->GetInformationObject( 0 );

  if(!inInfo)
    {
    vtkErrorMacro( << "Failed to get input information.");
    return 0;
    }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if(!inDataObj)
    {
    vtkErrorMacro( << "Failed to get input data object.");
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro( << "Failed to get output information.");
    }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!outDataObj)
    {
    vtkErrorMacro( << "Failed to get output data object.");
    }

  // Check if the input data is AMR.
  if(vtkHierarchicalBoxDataSet::SafeDownCast(inDataObj))
    {
    // If using scalars for clipping this should be NULL.
    if(!this->GetClipFunction())
      {
      // This is a lot to go through to get the name of the array to process.
      vtkInformationVector *inArrayVec =
        this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());
      if (!inArrayVec)
        {
        vtkErrorMacro("Problem finding array to process");
        return 1;
        }
      vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);
      if (!inArrayInfo)
        {
        vtkErrorMacro("Problem getting name of array to process.");
        return 1;
        }
      if ( ! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
        {
        vtkErrorMacro("Missing field name.");
        return 1;
        }
      const char *arrayNameToProcess =
        inArrayInfo->Get(vtkDataObject::FIELD_NAME());

      if(!arrayNameToProcess)
        {
        vtkErrorMacro("Unable to find valid array.");
        return 1;
        }

      vtkSmartPointer<vtkAMRDualClip> amrDC =
        vtkSmartPointer<vtkAMRDualClip>::New();
      amrDC->SetIsoValue(this->GetValue());

      // These default are safe to consider. Currently using GUI element just
      // for AMRDualClip filter enables all of these too.
      amrDC->SetEnableMergePoints(1);
      amrDC->SetEnableDegenerateCells(1);
      amrDC->SetEnableMultiProcessCommunication(1);

      amrDC->SetInput(0, inDataObj);
      amrDC->SetInputArrayToProcess(
        0, 0, 0, inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()),
        arrayNameToProcess);

      amrDC->Update();

      vtkMultiBlockDataSet::SafeDownCast(outDataObj)->ShallowCopy(
        amrDC->GetOutput(0));

      return 1;
      }
    else
      {
      vtkErrorMacro("This algorithm allows clipping using scalars only.");
      return 1;
      }
    }

  return Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestDataObject(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
    {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
      {
      output = vtkMultiBlockDataSet::New();
      output->SetPipelineInformation(outInfo);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
      }
    return 1;
    }
  else
    {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
      {
      output = vtkUnstructuredGrid::New();
      output->SetPipelineInformation(outInfo);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
      }
    return 1;
    }
}


//----------------------------------------------------------------------------
int vtkPVClipDataSet::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillInputPortInformation(int port,
                                               vtkInformation * info)
{
  this->Superclass::FillInputPortInformation(port, info);
  vtkInformationStringVectorKey::SafeDownCast(info->GetKey(
    vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))->Append(
    info, "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillOutputPortInformation(int port, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkObject");
  return 1;
}
