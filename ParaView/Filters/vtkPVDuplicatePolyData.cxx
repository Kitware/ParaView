/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDuplicatePolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDuplicatePolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"
#include "vtkTiledDisplaySchedule.h"

vtkCxxRevisionMacro(vtkPVDuplicatePolyData, "1.7.2.1");
vtkStandardNewMacro(vtkPVDuplicatePolyData);

vtkCxxSetObjectMacro(vtkPVDuplicatePolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkPVDuplicatePolyData,SocketController, vtkSocketController);

//-----------------------------------------------------------------------------
vtkPVDuplicatePolyData::vtkPVDuplicatePolyData()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  

  this->Schedule = vtkTiledDisplaySchedule::New();

  this->SocketController = NULL;
  this->ClientFlag = 0;
  this->PassThrough = 0;
  this->ZeroEmpty = 0;
}

//-----------------------------------------------------------------------------
vtkPVDuplicatePolyData::~vtkPVDuplicatePolyData()
{
  if (this->Schedule)
    {
    this->Schedule->Delete();
    }

  this->SetController(0);
  this->SetSocketController(0);
}


//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::InitializeSchedule(int numTiles)
{
  int numProcs = 1;

  if (this->Controller)
    {
    numProcs = this->Controller->GetNumberOfProcesses();
    }
  this->Schedule->InitializeTiles(numTiles, numProcs-this->ZeroEmpty);
}

//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkPolyData *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }
  input->SetUpdatePiece(piece);
  input->SetUpdateNumberOfPieces(numPieces);
  input->SetUpdateGhostLevel(ghostLevel);
}

  
//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int myId;
  int idx, tileId, otherProcessId;
  int numElements;
  // A list of appends (not all are used by all processes.
  vtkAppendPolyData** appendFilters;
  vtkPolyData* tmp;

  if (input == NULL)
    {
    vtkErrorMacro("Input has not been set.");
    return;
    }

  // Take this path if memory size is too large.
  if (this->PassThrough)
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }

  // Remote (socket) process as client.
  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute(this->SocketController);
    return;
    }
  // MPIRoot as client.
  // Subset of satellites for zero empty.
  if (this->Controller)
    {
    myId = this->Controller->GetLocalProcessId() - this->ZeroEmpty;
    }
  else
    {
    myId = 0;
    }
  if (myId < 0)
    {
    this->ClientExecute(this->Controller);
    return;
    }

  appendFilters = new vtkAppendPolyData* [this->Schedule->GetNumberOfTiles()];
  for (idx = 0; idx < this->Schedule->GetNumberOfTiles(); ++idx)
    {
    appendFilters[idx] = NULL;
    }

  // For zeroEmpty condition.
  numElements = this->Schedule->GetNumberOfProcessElements(myId);
  for (idx = 0; idx < numElements; ++idx)
    {
    otherProcessId = this->Schedule->GetElementOtherProcessId(myId, idx);
    if (this->Schedule->GetElementReceiveFlag(myId, idx))
      {
      tileId = this->Schedule->GetElementTileId(myId, idx);
      if (appendFilters[tileId] == NULL)
        {
        appendFilters[tileId] = vtkAppendPolyData::New();
        tmp = vtkPolyData::New();
        tmp->CopyStructure(input);
        tmp->GetPointData()->PassData(input->GetPointData());
        tmp->GetCellData()->PassData(input->GetCellData());
        appendFilters[tileId]->AddInput(tmp);
        tmp->Delete();
        tmp = NULL;
        }
      tmp = vtkPolyData::New();
      // +1 is for zeroEmpty condition.
      this->Controller->Receive(tmp, otherProcessId+this->ZeroEmpty, 12329);
      appendFilters[tileId]->AddInput(tmp);
      tmp->Delete();
      tmp = NULL;
      }
    else
      {
      tileId = this->Schedule->GetElementTileId(myId, idx);
      if (appendFilters[tileId] == NULL)
        {
        // +1 is for zeroEmpty condition.
        this->Controller->Send(input, otherProcessId+this->ZeroEmpty, 12329);
        }
      else
        {
        tmp = appendFilters[tileId]->GetOutput();
        tmp->Update();
        // +1 is for zeroEmpty condition.
        this->Controller->Send(tmp, otherProcessId+this->ZeroEmpty, 12329);
        // No longer need this filter.
        appendFilters[tileId]->Delete();
        appendFilters[tileId] = NULL;
        }
      }
    }

  // If we are a tile, copy to output.
  tileId = this->Schedule->GetProcessTileId(myId);
  if (tileId > -1)
    {
    if (appendFilters[tileId])
      {
      tmp = appendFilters[tileId]->GetOutput();
      tmp->Update();
      }
    else
      {
      tmp = input;
      }
    output->CopyStructure(tmp);
    output->GetPointData()->PassData(tmp->GetPointData());
    output->GetCellData()->PassData(tmp->GetCellData());
    }

  // Clean up temporary objects.
  for (idx = 0; idx < this->Schedule->GetNumberOfTiles(); ++idx)
    {
    if (appendFilters[idx])
      {
      appendFilters[idx]->Delete();
      appendFilters[idx] = NULL;
      }
    }
  delete [] appendFilters;

  // Send final results to client
  // Remember: myId may have been decremented.
  if (myId == 0)
    {
    if (this->ZeroEmpty)
      {
      this->Controller->Send(output, 0, 11872);
      }
    else
      {
      this->SocketController->Send(output, 1, 11872);
      }
    }
}


//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ClientExecute(vtkMultiProcessController* controller)
{
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *tmp = vtkPolyData::New();

  // No data is on the client, so we just have to get the data
  // from node 0 of the server.
  controller->Receive(tmp, 1, 11872);
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());
  tmp->Delete();
  tmp = NULL;
}



//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller: (" << this->Controller << ")\n";
  if (this->SocketController)
    {
    os << indent << "SocketController: (" << this->SocketController << ")\n";
    os << indent << "ClientFlag: " << this->ClientFlag << endl;
    }

  if (this->Schedule)
    {
    this->Schedule->PrintSelf(os, indent);
    }

  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "ZeroEmpty: " << this->ZeroEmpty << endl;
}

