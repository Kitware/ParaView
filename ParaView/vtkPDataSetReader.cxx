/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPDataSetReader.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPDataSetReader* vtkPDataSetReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPDataSetReader");
  if(ret)
    {
    return (vtkPDataSetReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPDataSetReader;
}


vtkPDataSetReader::vtkPDataSetReader()
{
  this->Controller = NULL;
}


vtkPDataSetReader::~vtkPDataSetReader()
{
  this->SetController(NULL);
}


void vtkPDataSetReader::Execute()
{
  int myId, numProcs, i;
  vtkDataSet *output;
  int extent[3];
  
  if (this->Controller == NULL)
    {
    this->SetController(vtkMultiProcessController::GetGlobalController());
    }
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();
  
  if (myId == 0)
    {
    this->vtkDataSetReader::Execute();
    output = this->GetOutput();
    const char *dataType = output->GetClassName();
    int length = strlen(dataType) + 1;
    // Tell the other processes the type so they do not have to update.
    for (i = 1; i < numProcs; ++i)
      {
      controller->Send(&length, 1, i, 89022);
      controller->Send(dataType, length, i, 89023);
      }
    
    if (strcmp(dataType,"vtkPolyData") == 0)
      {
      // Disconnect from the rest of the pipeline for safety.
      vtkPolyData *tmp = vtkPolyData::New();
      tmp->ShallowCopy(output);
      // Create the fitler which will split up the data.
      vtkExtractPolyDataPiece *extract = vtkExtractPolyDataPiece::New();
      extract->SetInput(tmp);
      // For each processes.
      for (i = 1; i < numProcs; ++i)
	{
	// Get the piece requested.
	controller->Receive(extent, 3, i, 89023);
	extract->GetOutput()->SetUpdateExtent(extent[0], extent[1], extent[2]);
	extract->Update();
	// Send the piece requested.
	controller->Send(extract->GetOutput(), i, 89024);
	}
      // Take care of the request in this process (0).
      extract->GetOutput()->SetUpdateExtent(output->GetUpdatePiece(), 
					    output->GetUpdateNumberOfPieces(),
					    output->GetUpdateGhostLevel());
      extract->Update();
      output->ShallowCopy(extract->GetOutput());
      tmp->Delete();
      extract->Delete();
      }
    } 
  else 
    {
    char *dataType;
    int length;
    int extent[3];
    // Receive the type of the output from proc 0.
    controller->Receive(&length, 1, 0, 89022);
    dataType = new char[length];
    controller->Receive(dataType, length, 0, 89023);
    
    if (strcmp(dataType,"vtkPolyData"))
      {
      vtkPolyData *tmp = this->Outputs[0];
      if (tmp == NULL)
	{
	tmp = vtkPolyData::New();
	this->Outputs[0] = tmp;
	tmp->Register(this);
	tmp->Delete();
	}
      // Send the piece we need.
      tmp->ShallowCopy(output);
      vtkExtractPolyDataPiece *extract = vtkExtractPolyDataPiece::New();
      extract->SetInput(tmp);
      // For each processes.
      for (i = 1; i < numProcs; ++i)
	{
	// Get the piece requested.
	controller->Receive(extent, 3, i, 89023);
	extract->GetOutput()->SetUpdateExtent(extent[0], extent[1], extent[2]);
	extract->Update();
	controller->Send(extract->GetOutput(), i, 89024);
	}
      // Take care of the request in this process (0).
      extract->GetOutput()->SetUpdateExtent(output->GetUpdatePiece(), 
					    output->GetUpdateNumberOfPieces(),
					    output->GetUpdateGhostLevel());
      extract->Update();
      output->ShallowCopy(extract->GetOutput());
      tmp->Delete();
      extract->Delete();
      }
    else
      { // Just let the normal update handle the request.
      this->vtkDataSetReader::Execute();
      } 
    delete [] dataType;
    }
}

void vtkPDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetReader::PrintSelf(os,indent);

  os << indent << "Controller (" << this->Controller << ")\n";
}
