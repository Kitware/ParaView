/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVUpdateSuppressor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVUpdateSuppressor.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPVUpdateSuppressor, "1.5");
vtkStandardNewMacro(vtkPVUpdateSuppressor);

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->vtkSource::SetNthOutput(1,vtkPolyData::New());
  this->Outputs[1]->Delete();

  this->UpdatePiece = 0;
  this->UpdateNumberOfPieces = 1;
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
}

//----------------------------------------------------------------------------
unsigned long vtkPVUpdateSuppressor::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataToPolyDataFilter::GetMTime();

  return mTime;
}


// All these recursive pipeline methods now do nothing.
// I could use the UpdateExtent from the output, but
// The user may not have called update before force update.

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::UpdateData(vtkDataObject *)
{
}
//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::UpdateInformation()
{
}
//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PropagateUpdateExtent(vtkDataObject *)
{
}
//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::TriggerAsynchronousUpdate()
{
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::ForceUpdate()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  input->SetUpdatePiece(this->UpdatePiece);
  input->SetUpdateNumberOfPieces(this->UpdateNumberOfPieces);
  input->Update();
  if (input->GetPipelineMTime() > this->UpdateTime || output->GetDataReleased())
    {
    output->ShallowCopy(input);
    this->UpdateTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  if (!input || !output)
    {
    return;
    }  
  output->ShallowCopy(input);
}


//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UpdatePiece: " << this->UpdatePiece << endl;
  os << indent << "UpdateNumberOfPieces: " << this->UpdateNumberOfPieces << endl;
}
