/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHDataToCTHDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHDataToCTHDataFilter.h"

#include "vtkCTHData.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"

vtkCxxRevisionMacro(vtkCTHDataToCTHDataFilter, "1.1");

//----------------------------------------------------------------------------
vtkCTHDataToCTHDataFilter::vtkCTHDataToCTHDataFilter()
{
  this->SetNumberOfInputPorts(1);
  this->NumberOfRequiredInputs = 1;
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkCTHDataToCTHDataFilter::SetInput(vtkCTHData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkCTHData *vtkCTHDataToCTHDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkCTHData *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkCTHDataToCTHDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input)
    {
    this->vtkCTHSource::ComputeInputUpdateExtents(output);
    input->RequestExactExtentOn();
    }
}

//----------------------------------------------------------------------------
int vtkCTHDataToCTHDataFilter::FillInputPortInformation(int port,
                                                          vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  //info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCTHData"); HACK
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCTHData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCTHDataToCTHDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
