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
#include "vtkCollection.h"

vtkCxxRevisionMacro(vtkPVUpdateSuppressor, "1.7");
vtkStandardNewMacro(vtkPVUpdateSuppressor);

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->vtkSource::SetNthOutput(1,vtkPolyData::New());
  this->Outputs[1]->Delete();

  this->UpdatePiece = 0;
  this->UpdateNumberOfPieces = 1;

  this->CachedGeometry = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
  this->CachedGeometry->Delete();
  this->CachedGeometry = NULL;
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

  // Assume the input is the collection filter.
  // Client needs to modify the collection filter because it is not
  // connected to a pipeline.
  if (input && input->GetSource())
    {
    input->GetSource()->Modified();
    }

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
void vtkPVUpdateSuppressor::RemoveAllCaches()
{
  this->CachedGeometry->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::CacheUpdate(int idx)
{
  vtkPolyData *pd;
  vtkPolyData *output;

  output = this->GetOutput();
  pd = static_cast<vtkPolyData*>(this->CachedGeometry->GetItemAsObject(idx));
  if (pd == NULL)
    { // we need to update and save.
    this->ForceUpdate();
    pd = vtkPolyData::New();
    pd->ShallowCopy(output);
    this->CachedGeometry->ReplaceItem(idx, pd);
    pd->Delete();
    }
  else
    { // Output generated previously.
    output->ShallowCopy(pd);
    }
}


//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UpdatePiece: " << this->UpdatePiece << endl;
  os << indent << "UpdateNumberOfPieces: " << this->UpdateNumberOfPieces << endl;
}
