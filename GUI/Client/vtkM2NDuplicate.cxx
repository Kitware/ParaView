/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkM2NDuplicate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkM2NDuplicate.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyDataWriter.h"
#include "vtkPolyDataReader.h"
#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#include "vtkAllToNRedistributePolyData.h"
#endif
#include "vtkMPIMToNSocketConnection.h"
#include "vtkSocketCommunicator.h"

vtkCxxRevisionMacro(vtkM2NDuplicate, "1.1");
vtkStandardNewMacro(vtkM2NDuplicate);

vtkCxxSetObjectMacro(vtkM2NDuplicate,MPIMToNSocketConnection, vtkMPIMToNSocketConnection);

//-----------------------------------------------------------------------------
vtkM2NDuplicate::vtkM2NDuplicate()
{
  // Client has no inputs.
  this->NumberOfRequiredInputs = 0;
  this->ServerMode =0;
  this->RenderServerMode =0;
  this->ClientMode = 0;
  this->MPIMToNSocketConnection = 0;
}

//-----------------------------------------------------------------------------
vtkM2NDuplicate::~vtkM2NDuplicate()
{
}


//-----------------------------------------------------------------------------
void vtkM2NDuplicate::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkM2NDuplicate::ComputeInputUpdateExtents(vtkDataObject *output)
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


/*
Here is the new plan:

1: Assume there are fewer render processes than data processes.
2: Do the M to N conversion on the data server.
3: If there are N sockets, then the communication is trivial

Handle this case later!
4: If there are fewer sockets, then .....

Two phases:
Phase 1: Transfer 1 to 1 data on processes with sockets.
Phase 2: 
A: The last processes on thedata server is the master/scheduler.
B: All processes with sockets ask the last process for a task.
C: The last process starts assigning and checking off processes with data but no socket.
D: If the last process has data, then it assigns its own data last.
E: All processes with data but no socket ask/wait on a receiveto find out 
   which process to send its data to.
F: All processes with a socket wait in a while loop for a message from
   the last process.  This message tells thesocket process who to receive data from.
G: When the last process has finish assigning task, it send  a message to all
   the socket processes that they are finished.

*/  

//-----------------------------------------------------------------------------
void vtkM2NDuplicate::Execute()
{
  // If no render server, handle compositing with super(which will do nothing).
  if (this->MPIMToNSocketConnection == NULL || this->ClientMode)
    {
    this->Superclass::Execute();
    return;
    }
  
  vtkPolyData* output = this->GetOutput();
  vtkMultiProcessController* controller = this->GetController();
  int bufSize = 0;
  char* buf = NULL;
  vtkSocketCommunicator* com = 
      this->MPIMToNSocketConnection->GetSocketCommunicator();
  if (this->PassThrough)
    {
    if (this->RenderServerMode)
      {
      if (com == NULL)
        {
        vtkErrorMacro("Missing communicator.");
        return;
        }
      com->Receive(&bufSize, 1, 1, 55436);
      if (bufSize > 0)
        {
        buf = new char[bufSize];
        com->Receive(buf, bufSize, 1, 55437);
        // Setup a reader.
        vtkPolyDataReader *reader = vtkPolyDataReader::New();
        reader->ReadFromInputStringOn();
        vtkCharArray* mystring = vtkCharArray::New();
        mystring->SetArray(buf, bufSize, 1);
        reader->SetInputArray(mystring);
        reader->Modified(); // For append loop
        reader->GetOutput()->Update();
        vtkPolyData* pd = reader->GetOutput();
        vtkPolyData* output = this->GetOutput();
        output->CopyStructure(pd);
        output->GetPointData()->PassData(pd->GetPointData());
        output->GetCellData()->PassData(pd->GetCellData());
        mystring->Delete();
        mystring = NULL;
        reader->Delete();
        reader = NULL;
        delete [] buf;
        buf = NULL;
        }
      return;
      }
    // This has to be data server mode.
    if ( ! this->ServerMode)
      {
      vtkErrorMacro("Mode mixup.");
      return;
      }
    // Execute All to n and send to render server.
    int numConnections=this->MPIMToNSocketConnection->GetNumberOfConnections();
    int bufSize = 0;
    char* buf = NULL;
    vtkPolyData* input = this->GetInput();

    // If we are the data server and there are fewer render processes,
    // Perform the M to N operation.
#ifdef VTK_USE_MPI
    vtkAllToNRedistributePolyData* AllToN = NULL;
    if (controller && controller->GetNumberOfProcesses() > numConnections)
      {
      vtkPolyData* inputCopy = vtkPolyData::New();
      inputCopy->ShallowCopy(input);
      AllToN = vtkAllToNRedistributePolyData::New();
      AllToN->SetController(controller);
      AllToN->SetNumberOfProcesses(numConnections);
      AllToN->SetInput(inputCopy);
      inputCopy->Delete();
      input = AllToN->GetOutput();
      input->SetUpdateNumberOfPieces(this->GetOutput()->GetUpdateNumberOfPieces());
      input->SetUpdatePiece(this->GetOutput()->GetUpdatePiece());
      input->Update();
      }
#endif

    // If we have data, marshal it.
    if (com)
      {
      // Copy input to isolate reader from the pipeline.
      vtkPolyData* pd = vtkPolyData::New();
      pd->CopyStructure(input);
      pd->GetPointData()->PassData(input->GetPointData());
      pd->GetCellData()->PassData(input->GetCellData());
      // Marshal with writer.
      vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
      writer->SetFileTypeToBinary();
      writer->WriteToOutputStringOn();
      writer->SetInput(pd);
      writer->Write();
      // Get string.
      bufSize = writer->GetOutputStringLength();
      buf = writer->RegisterAndGetOutputString();
      com->Send(&bufSize, 1, 1, 55436);
      if (bufSize > 0)
        {
        com->Send(buf, bufSize, 1, 55437);
        }
      // Get rid of writer and copy of input.
      writer->Delete();
      writer = NULL;
      pd->Delete();
      pd = NULL;
      }

#ifdef VTK_USE_MPI
    if (AllToN)
      {
      AllToN->Delete();
      AllToN = NULL;
      input = NULL;
      }
#endif
    }


  // Not pass through (and not client).
  if (! this->RenderServerMode)
    {
    this->Superclass::Execute();
    // Only node zero sends the data.
    if (controller->GetLocalProcessId() == 0)
      {
      if ( com == 0)
        {
        vtkErrorMacro("Missing communicator.");
        return;
        }
      // Copy output to isolate reader from the pipeline.
      vtkPolyData* pd = vtkPolyData::New();
      pd->CopyStructure(output);
      pd->GetPointData()->PassData(output->GetPointData());
      pd->GetCellData()->PassData(output->GetCellData());
      // Marshal with writer.
      vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
      writer->SetFileTypeToBinary();
      writer->WriteToOutputStringOn();
      writer->SetInput(pd);
      writer->Write();
      // Get string.
      bufSize = writer->GetOutputStringLength();
      buf = writer->RegisterAndGetOutputString();
      com->Send(&bufSize, 1, 1, 55438);
      if (bufSize > 0)
        {
        com->Send(buf, bufSize, 1, 55439);
        }
      // Get rid of writer and copy of input.
      writer->Delete();
      writer = NULL;
      pd->Delete();
      pd = NULL;
      }
    return;
    }

  // Render server and not duplicate.
  if (controller->GetLocalProcessId() == 0)
    {
    if ( com == 0)
      {
      vtkErrorMacro("Missing communicator.");
      return;
      }
    com->Receive(&bufSize, 1, 1, 55438);
    if (bufSize > 0)
      {
      buf = new char[bufSize];
      com->Receive(buf, bufSize, 1, 55439);
      }
    // Broadcast to all procs of render server.
    int i, num;
    num = controller->GetNumberOfProcesses();
    for (i = 1; i < num; ++i)
      {
      this->Controller->Send(&bufSize, 1, i, 55440);
      if (bufSize > 0)
        {
        this->Controller->Send(buf, bufSize, i, 55441);
        }
      }
    }
  else
    {
    this->Controller->Receive(&bufSize, 1, 0, 55440);
    if (bufSize > 0)
      {
      buf = new char[bufSize];
      this->Controller->Receive(buf, bufSize, 0, 55441);
      }
    }
     
  // Reconstruct output.
  if (bufSize > 0)
    {
    vtkPolyDataReader *reader = vtkPolyDataReader::New();
    reader->ReadFromInputStringOn();
    vtkCharArray* mystring = vtkCharArray::New();
    mystring->SetArray(buf, bufSize, 1);
    reader->SetInputArray(mystring);
    reader->Modified(); // For append loop
    reader->GetOutput()->Update();
    vtkPolyData* pd = reader->GetOutput();
    vtkPolyData* output = this->GetOutput();
    output->CopyStructure(pd);
    output->GetPointData()->PassData(pd->GetPointData());
    output->GetCellData()->PassData(pd->GetCellData());
    mystring->Delete();
    mystring = NULL;
    reader->Delete();
    reader = NULL;
    delete [] buf;
    buf = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkM2NDuplicate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

