/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUpdateSuppressorPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUpdateSuppressorPipeline.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkProcessModule.h"

vtkCxxRevisionMacro(vtkUpdateSuppressorPipeline, "1.7");
vtkStandardNewMacro(vtkUpdateSuppressorPipeline);


//----------------------------------------------------------------------------
vtkUpdateSuppressorPipeline::vtkUpdateSuppressorPipeline()
{
}

//----------------------------------------------------------------------------
vtkUpdateSuppressorPipeline::~vtkUpdateSuppressorPipeline()
{
}

//----------------------------------------------------------------------------
void vtkUpdateSuppressorPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkUpdateSuppressorPipeline::ProcessRequest(vtkInformation* request,
                                                vtkInformationVector** inInfo,
                                                vtkInformationVector* outInfo)
{
  if(this->Algorithm && request->Has(REQUEST_DATA()))
    {
    // If the number of pieces that was requested is larger than 1,
    // paraview is running in streaming mode. Pass the request through
    // so that the filters can execute on the requested piece.
    vtkInformation *info = outInfo->GetInformationObject(0);
    int numPieces = info->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    if (numPieces == 1)
      {
      return 1;
      }
    }
  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation* info = outInfo->GetInformationObject(0);
    if(!info->Has(MAXIMUM_NUMBER_OF_PIECES()))
      {
      info->Set(MAXIMUM_NUMBER_OF_PIECES(), -1);
      }
    // If the number of pieces that was requested is larger than 1,
    // paraview is running in streaming mode. Pass the request through
    // so that the filters can execute on the requested piece.
    int numPieces = info->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    if (numPieces == 1)
      {
      return 1;
      }
    }
  return this->Superclass::ProcessRequest(request, inInfo, outInfo);
}



