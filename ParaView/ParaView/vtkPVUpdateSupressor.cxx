/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVUpdateSupressor.cxx
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
#include "vtkPVUpdateSupressor.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVUpdateSupressor, "1.2");
vtkStandardNewMacro(vtkPVUpdateSupressor);

//----------------------------------------------------------------------------
vtkPVUpdateSupressor::vtkPVUpdateSupressor()
{
  this->vtkSource::SetNthOutput(1,vtkPolyData::New());
  this->Outputs[1]->Delete();
}

//----------------------------------------------------------------------------
vtkPVUpdateSupressor::~vtkPVUpdateSupressor()
{
}

//----------------------------------------------------------------------------
unsigned long vtkPVUpdateSupressor::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataToPolyDataFilter::GetMTime();

  return mTime;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSupressor::ForceUpdate()
{
   vtkPolyData *input = this->GetInput();
   if ( input )
     {
     //cout << "Do update on: " << input << endl;
     input->Update();
     }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSupressor::UpdateData(vtkDataObject *output)
{
  int idx;

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  // Propagate the update call - make sure everything we
  // might rely on is up-to-date
  // Must call PropagateUpdateExtent before UpdateData if multiple 
  // inputs since they may lead back to the same data object.
  this->Updating = 1;
  if ( this->NumberOfInputs == 1 )
    {
    if (this->Inputs[0] != NULL)
      {
      // Do not update input
      //this->Inputs[0]->UpdateData();
      }
    }
  else
    { // To avoid serlializing execution of pipelines with ports
    // we need to sort the inputs by locality (ascending).
    this->SortInputsByLocality();
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      if (this->SortedInputs[idx] != NULL)
        {
        this->SortedInputs[idx]->PropagateUpdateExtent();
	// Do not update input
        //this->SortedInputs[idx]->UpdateData();
        }
      }
    }
  this->Updating = 0;     
    
  // Initialize all the outputs
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->PrepareForNewData(); 
      }
    }
 
  // If there is a start method, call it
  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  // Execute this object - we have not aborted yet, and our progress
  // before we start to execute is 0.0.
  this->AbortExecute = 0;
  this->Progress = 0.0;
  if (this->NumberOfInputs < this->NumberOfRequiredInputs)
    {
    vtkErrorMacro(<< "At least " << this->NumberOfRequiredInputs << " inputs are required but only " << this->NumberOfInputs << " are specified");
    }
  else
    {
    this->ExecuteData(output);
    // Pass the vtkDataObject's field data from the first input
    // to all outputs
    vtkFieldData* fd;
    if ((this->NumberOfInputs > 0) && (this->Inputs[0]) && 
        (fd = this->Inputs[0]->GetFieldData()))
      {
      vtkFieldData* outputFd;
      for (idx = 0; idx < this->NumberOfOutputs; idx++)
        {
        if (this->Outputs[idx] && 
            (outputFd=this->Outputs[idx]->GetFieldData()))
          {
          outputFd->PassData(fd);
          }
        }
      }
    }

  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
    
  // Now we have to mark the data as up to data.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->DataHasBeenGenerated();
      }
    }
  
  // Release any inputs if marked for release
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if ( this->Inputs[idx]->ShouldIReleaseData() )
        {
        this->Inputs[idx]->ReleaseData();
        }
      }  
    }
  
}

//----------------------------------------------------------------------------
void vtkPVUpdateSupressor::Execute()
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
void vtkPVUpdateSupressor::SetUpdateNumberOfPieces(int p)
{
  vtkPolyData *input = this->GetInput();
  if ( input )
    {
    input->SetUpdateNumberOfPieces(p);
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSupressor::SetUpdatePiece(int p)
{
  vtkPolyData *input = this->GetInput();
  if ( input )
    {
    input->SetUpdatePiece(p);
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSupressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
