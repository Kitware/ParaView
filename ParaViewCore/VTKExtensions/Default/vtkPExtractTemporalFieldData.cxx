/*=========================================================================

  Program:   ParaView
  Module:    vtkPExtractTemporalFieldData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractTemporalFieldData.h"

#include "vtkCompositeDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPExtractTemporalFieldData);
vtkCxxSetObjectMacro(vtkPExtractTemporalFieldData, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPExtractTemporalFieldData::vtkPExtractTemporalFieldData()
  : Controller(NULL)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPExtractTemporalFieldData::~vtkPExtractTemporalFieldData()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkPExtractTemporalFieldData::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->Controller == NULL || this->Controller->GetLocalProcessId() == 0)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // On all other ranks, we do nothing, except it both input & output are
  // composite datasets. In that case, we copy structure to avoid composite data
  // structure mismatch between ranks.
  vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector, 0);
  if (input && output)
  {
    output->CopyStructure(input);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPExtractTemporalFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
