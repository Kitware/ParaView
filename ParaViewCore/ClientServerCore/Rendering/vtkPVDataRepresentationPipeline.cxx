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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"

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
  vtkPVDataRepresentation* representation = vtkPVDataRepresentation::SafeDownCast(this->Algorithm);
  if (representation && representation->GetUsingCacheForUpdate())
  {
    // shunt upstream updates when using cache.
    return 1;
  }

  if (representation && !representation->GetNeedUpdate())
  {
    // shunt upstream updates when using cache.
    return 1;
  }

  return this->Superclass::ForwardUpstream(i, j, request);
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentationPipeline::ForwardUpstream(vtkInformation* request)
{
  vtkPVDataRepresentation* representation = vtkPVDataRepresentation::SafeDownCast(this->Algorithm);
  if (representation && representation->GetUsingCacheForUpdate())
  {
    // shunt upstream updates when using cache.
    return 1;
  }

  if (representation && !representation->GetNeedUpdate())
  {
    // shunt upstream updates when using cache.
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
    vtkPVDataRepresentation* representation =
      vtkPVDataRepresentation::SafeDownCast(this->Algorithm);
    // This check doesn't make sense. We need to call RequestData() whenever
    // needed even when using cache. How else is the cache-keeper going to
    // produce a new timestep? (BUG #11321).
    // if (representation && representation->GetUsingCacheForUpdate())
    //  {
    //  // shunt upstream updates when using cache.
    //  return 1;
    //  }

    if (representation && !representation->GetNeedUpdate())
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
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentationPipeline::NeedToExecuteData(
  int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  return this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentationPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
