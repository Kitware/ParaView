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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkUpdateSuppressorPipeline, "1.1.2.1");
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
int vtkUpdateSuppressorPipeline::ProcessRequest(vtkInformation* request)
{
  if(this->Algorithm && request->Has(REQUEST_DATA()))
    {
    return 1;
    }
  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation* info = this->GetOutputInformation(0);
    if(!info->Has(MAXIMUM_NUMBER_OF_PIECES()))
      {
      info->Set(MAXIMUM_NUMBER_OF_PIECES(), -1);
      }
    return 1;
    }
  return this->Superclass::ProcessRequest(request);
}



