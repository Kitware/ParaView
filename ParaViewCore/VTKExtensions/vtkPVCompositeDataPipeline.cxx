/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVCompositeDataPipeline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilterExecutive.h"
#include "vtkStreamingExtentTranslator.h"

#include <assert.h>

vtkStandardNewMacro(vtkPVCompositeDataPipeline);
vtkInformationKeyRestrictedMacro(vtkPVCompositeDataPipeline,
  STREAMING_EXTENT_TRANSLATOR, ObjectBase, "vtkStreamingExtentTranslator");
//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::~vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::CopyDefaultInformation(
  vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction,
    inInfoVec, outInfoVec);

  if (request->Has(REQUEST_INFORMATION()))
    {
    if (this->GetNumberOfInputPorts() > 0)
      {
      if (vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
        {
        // Copy information from the first input to all outputs.
        for (int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
          {
          vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          // Any new meta-data keys that we add to this class that need to be
          // passed down the pipeline need to be copied here.
          outInfo->CopyEntry(inInfo, STREAMING_EXTENT_TRANSLATOR());
          }
        }
      }
    }
  else if (request->Has(REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation* algorithmInfo = this->Algorithm->GetInformation();

    // All SetInputArrayToProcess() calls result in updating this algorithmInfo
    // object.
    vtkInformationVector *inArrayVec =
      algorithmInfo->Get(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS());
    int num_arrays =
      inArrayVec? inArrayVec->GetNumberOfInformationObjects(): 0;
    for (int array_index=0; array_index < num_arrays; array_index++)
      {
      vtkInformation* arrayInfo =
        this->Algorithm->GetInputArrayInformation(array_index);
      // currently, we only support conversion for array set using FIELD_NAME().
      if (arrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
        arrayInfo->Has(vtkAlgorithm::INPUT_PORT()) &&
        arrayInfo->Has(vtkAlgorithm::INPUT_CONNECTION()) &&
        arrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
        {
        int port = arrayInfo->Get(vtkAlgorithm::INPUT_PORT());
        int connection = arrayInfo->Get(vtkAlgorithm::INPUT_CONNECTION());
        if (port < 0 || port >= this->GetNumberOfInputPorts() ||
          connection < 0 || connection >= this->GetNumberOfInputConnections(port))
          {
          continue;
          }
        vtkExecutive* input_executive = this->GetInputExecutive(port, connection);
        vtkPVPostFilterExecutive *pvpfe =
          vtkPVPostFilterExecutive::SafeDownCast(input_executive);
        if (pvpfe)
          {
          assert(this->Algorithm->GetInputConnection(
              port, connection)->GetIndex() == 0);
          pvpfe->SetPostArrayToProcessInformation(0, arrayInfo);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::ResetPipelineInformation(
  int port, vtkInformation* info)
{
  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(STREAMING_EXTENT_TRANSLATOR());
}

//----------------------------------------------------------------------------
vtkStreamingExtentTranslator*
vtkPVCompositeDataPipeline::GetStreamingExtentTranslator(int port)
{
  return this->GetStreamingExtentTranslator(this->GetOutputInformation(port));
}

//----------------------------------------------------------------------------
vtkStreamingExtentTranslator*
vtkPVCompositeDataPipeline::GetStreamingExtentTranslator(vtkInformation* info)
{
  if (!info)
    {
    vtkGenericWarningMacro("Attempt to get translator for invalid output");
    return 0;
    }

  return vtkStreamingExtentTranslator::SafeDownCast(
    info->Get(STREAMING_EXTENT_TRANSLATOR()));
}

//----------------------------------------------------------------------------
int vtkPVCompositeDataPipeline::SetStreamingExtentTranslator(
  int port, vtkStreamingExtentTranslator* translator)
{
  return this->SetStreamingExtentTranslator(
    this->GetOutputInformation(port), translator);
}

//----------------------------------------------------------------------------
int vtkPVCompositeDataPipeline::SetStreamingExtentTranslator(
  vtkInformation *info, vtkStreamingExtentTranslator* translator)
{
  if (!info)
    {
    vtkGenericWarningMacro("Attempt to set translator for invalid output");
    return 0;
    }

  vtkStreamingExtentTranslator* oldTranslator =
    vtkStreamingExtentTranslator::SafeDownCast(info->Get(STREAMING_EXTENT_TRANSLATOR()));
  if (translator != oldTranslator)
    {
    info->Set(STREAMING_EXTENT_TRANSLATOR(), translator);
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVCompositeDataPipeline::SetUpdateStreamingExtent(
  int port, int pass, int number_of_passes)
{
  return this->SetUpdateStreamingExtent(
    this->GetOutputInformation(port), pass, number_of_passes);
}

//----------------------------------------------------------------------------
int vtkPVCompositeDataPipeline::SetUpdateStreamingExtent(
  vtkInformation* info, int pass, int number_of_passes)
{
  if (!info)
    {
    vtkGenericWarningMacro("SetUpdateStreamingExtent on invalid output");
    return 0;
    }

  // check if info has vtkStreamingExtentTranslator. If none is present, it
  // implies that the pipeline is not capable of streaming, so we leave the
  // update request unchanged.
  vtkStreamingExtentTranslator* translator =
    vtkPVCompositeDataPipeline::GetStreamingExtentTranslator(info);
  if (!translator)
    {
    return 0;
    }

  // Use the translator provided by data-producer to make a relevant request.
  return translator->PassToRequest(pass, number_of_passes, info);
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
