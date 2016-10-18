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

vtkStandardNewMacro(vtkUpdateSuppressorPipeline);

//----------------------------------------------------------------------------
vtkUpdateSuppressorPipeline::vtkUpdateSuppressorPipeline()
{
  this->Enabled = true;
}

//----------------------------------------------------------------------------
vtkUpdateSuppressorPipeline::~vtkUpdateSuppressorPipeline()
{
}

//----------------------------------------------------------------------------
void vtkUpdateSuppressorPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
}

//----------------------------------------------------------------------------
int vtkUpdateSuppressorPipeline::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  if (this->Enabled)
  {
    if (this->Algorithm && request->Has(REQUEST_DATA()))
    {
      return 1;
    }
    if (request->Has(REQUEST_UPDATE_EXTENT()))
    {
      return 1;
    }
  }
  return this->Superclass::ProcessRequest(request, inInfo, outInfo);
}
