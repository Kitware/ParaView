/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHDataToPolyDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHDataToPolyDataFilter.h"

#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCTHDataToPolyDataFilter, "1.3");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkCTHDataToPolyDataFilter::SetInput(vtkCTHData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkCTHData *vtkCTHDataToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkCTHData *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkCTHDataToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input)
    {
    this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
    input->RequestExactExtentOn();
    }
}

//----------------------------------------------------------------------------
void vtkCTHDataToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
