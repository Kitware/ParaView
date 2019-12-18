/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentationPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataRepresentationPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVDataRepresentationPipeline);
//----------------------------------------------------------------------------
vtkPVDataRepresentationPipeline::vtkPVDataRepresentationPipeline()
{
}

//----------------------------------------------------------------------------
vtkPVDataRepresentationPipeline::~vtkPVDataRepresentationPipeline()
{
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentationPipeline::ForwardUpstream(int i, int j, vtkInformation* request)
{
  if (!this->NeedsUpdate)
  {
    // shunt upstream updates unless explicitly requested.
    return 1;
  }

  return this->Superclass::ForwardUpstream(i, j, request);
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentationPipeline::ForwardUpstream(vtkInformation* request)
{
  if (!this->NeedsUpdate)
  {
    // shunt upstream updates unless explicitly requested.
    return 1;
  }

  return this->Superclass::ForwardUpstream(request);
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentationPipeline::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  if (request->Has(REQUEST_DATA()) || request->Has(REQUEST_UPDATE_EXTENT()))
  {
    if (!this->NeedsUpdate)
    {
      // shunt upstream updates when using cache.
      return 1;
    }
  }

  return this->Superclass::ProcessRequest(request, inInfo, outInfo);
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentationPipeline::ExecuteDataEnd(
  vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  this->Superclass::ExecuteDataEnd(request, inInfoVec, outInfoVec);
  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->Algorithm->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentationPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
