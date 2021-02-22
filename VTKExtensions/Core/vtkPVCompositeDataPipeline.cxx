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

#include <assert.h>

vtkStandardNewMacro(vtkPVCompositeDataPipeline);
//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::vtkPVCompositeDataPipeline() = default;

//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::~vtkPVCompositeDataPipeline() = default;

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::CopyDefaultInformation(vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction, inInfoVec, outInfoVec);

  if (request->Has(REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* algorithmInfo = this->Algorithm->GetInformation();

    // All SetInputArrayToProcess() calls result in updating this algorithmInfo
    // object.
    vtkInformationVector* inArrayVec = algorithmInfo->Get(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS());
    int num_arrays = inArrayVec ? inArrayVec->GetNumberOfInformationObjects() : 0;
    int informationIndex = 0;
    for (int array_index = 0; array_index < num_arrays; array_index++)
    {
      vtkInformation* arrayInfo = this->Algorithm->GetInputArrayInformation(array_index);
      // currently, we only support conversion for array set using FIELD_NAME().
      if (arrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
        arrayInfo->Has(vtkAlgorithm::INPUT_PORT()) &&
        arrayInfo->Has(vtkAlgorithm::INPUT_CONNECTION()) &&
        arrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
      {
        int port = arrayInfo->Get(vtkAlgorithm::INPUT_PORT());
        int connection = arrayInfo->Get(vtkAlgorithm::INPUT_CONNECTION());
        if (port < 0 || port >= this->GetNumberOfInputPorts() || connection < 0 ||
          connection >= this->GetNumberOfInputConnections(port))
        {
          continue;
        }
        vtkExecutive* input_executive = this->GetInputExecutive(port, connection);
        vtkPVPostFilterExecutive* pvpfe = vtkPVPostFilterExecutive::SafeDownCast(input_executive);
        if (pvpfe)
        {
          assert(this->Algorithm->GetInputConnection(port, connection)->GetIndex() == 0);
          pvpfe->SetPostArrayToProcessInformation(informationIndex, arrayInfo);
          informationIndex++;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::ResetPipelineInformation(int port, vtkInformation* info)
{
  this->Superclass::ResetPipelineInformation(port, info);
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
