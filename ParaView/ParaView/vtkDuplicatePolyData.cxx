/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDuplicatePolyData.cxx
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
#include "vtkDuplicatePolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkDuplicatePolyData, "1.2.2.1");
vtkStandardNewMacro(vtkDuplicatePolyData);

vtkCxxSetObjectMacro(vtkDuplicatePolyData,Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkDuplicatePolyData::vtkDuplicatePolyData()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
}

//----------------------------------------------------------------------------
vtkDuplicatePolyData::~vtkDuplicatePolyData()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkDuplicatePolyData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//--------------------------------------------------------------------------
void vtkDuplicatePolyData::ComputeInputUpdateExtents(vtkDataObject *output)
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

  
//----------------------------------------------------------------------------
void vtkDuplicatePolyData::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int numProcs, myId;
  int idx;

  if (input == NULL)
    {
    vtkErrorMacro("Input has not been set.");
    return;
    }

  if (this->Controller == NULL)
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  // Collect.
  vtkPolyData *pd = NULL;;

  if (myId == 0)
    {
    vtkAppendPolyData *append = vtkAppendPolyData::New();
    pd = vtkPolyData::New();
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    for (idx = 1; idx < numProcs; ++idx)
      {
      pd = vtkPolyData::New();
      this->Controller->Receive(pd, idx, 131767);
      append->AddInput(pd);
      pd->Delete();
      pd = NULL;
      }
    append->Update();
    input = append->GetOutput();

    // Send to all processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Send(input, idx, 131768);
      }

    // Copy to output.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    append->Delete();
    append = NULL;
    }
  else
    {
    this->Controller->Send(input, 0, 131767);
    vtkPolyData *pd = vtkPolyData::New();
    this->Controller->Receive(pd, 0, 131768);
    output->CopyStructure(pd);
    output->GetPointData()->PassData(pd->GetPointData());
    output->GetCellData()->PassData(pd->GetCellData());
    pd->Delete();
    pd = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkDuplicatePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller: (" << this->Controller << ")\n";
}

