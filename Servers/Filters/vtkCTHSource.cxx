/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHSource.h"
#include "vtkObjectFactory.h"
#include "vtkCTHData.h"


vtkCxxRevisionMacro(vtkCTHSource, "1.3");
vtkStandardNewMacro(vtkCTHSource);

//----------------------------------------------------------------------------
vtkCTHSource::vtkCTHSource()
{
  this->vtkSource::SetNthOutput(0, vtkCTHData::New());
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkCTHSource::~vtkCTHSource()
{
}

//----------------------------------------------------------------------------
vtkCTHData *vtkCTHSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkCTHData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkCTHData *vtkCTHSource::GetOutput(int idx)
{
  return static_cast<vtkCTHData *>( this->vtkSource::GetOutput(idx) ); 
}

//----------------------------------------------------------------------------
void vtkCTHSource::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkCTHData *output = (vtkCTHData*)data;
  int idx;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
  
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }
  
  if (ghostLevel < 0)
    {
    return;
    }
  
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->SetUpdateExtent(piece, numPieces, ghostLevel);
      }
    }
}

//----------------------------------------------------------------------------
void vtkCTHSource::SetOutput(vtkCTHData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkCTHSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

