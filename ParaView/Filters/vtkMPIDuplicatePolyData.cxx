/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIDuplicatePolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIDuplicatePolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkMPICommunicator.h"
#include "vtkPolyDataWriter.h"
#include "vtkPolyDataReader.h"
#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"

vtkCxxRevisionMacro(vtkMPIDuplicatePolyData, "1.2");
vtkStandardNewMacro(vtkMPIDuplicatePolyData);

vtkCxxSetObjectMacro(vtkMPIDuplicatePolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIDuplicatePolyData,SocketController, vtkSocketController);

//-----------------------------------------------------------------------------
vtkMPIDuplicatePolyData::vtkMPIDuplicatePolyData()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  

  this->SocketController = NULL;
  this->ClientFlag = 0;
  this->MemorySize = 0;
}

//-----------------------------------------------------------------------------
vtkMPIDuplicatePolyData::~vtkMPIDuplicatePolyData()
{
  this->SetController(0);
}


#define vtkDPDPow2(j) (1 << (j))
static inline int vtkDPDLog2(int j, int& exact)
{
  int counter=0;
  exact = 1;
  while(j)
    {
    if ( ( j & 1 ) && (j >> 1) )
      {
      exact = 0;
      }
    j = j >> 1;
    counter++;
    }
  return counter-1;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ComputeInputUpdateExtents(vtkDataObject *output)
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
void vtkMPIDuplicatePolyData::Execute()
{
  vtkPolyData *input = this->GetInput();
  int myId;

  if (input == NULL)
    { // This will probably lock up.
    vtkErrorMacro("Input has not been set.");
    return;
    }

  vtkMPICommunicator *com = vtkMPICommunicator::SafeDownCast(
                                 this->Controller->GetCommunicator());

  if (com == NULL)
    { // This will probably lock up.
    vtkErrorMacro("Could not get MPI communicator.");
    return;
    }

  // Setup a reader.
  vtkPolyDataReader *reader = vtkPolyDataReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* mystring = vtkCharArray::New();
  reader->SetInputArray(mystring);
  mystring->Delete();

  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute(reader);
    reader->Delete();
    reader = NULL;
    return;
    }

  // Create the writer.
  vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
  writer->WriteToOutputStringOn();

  myId = this->Controller->GetLocalProcessId();
  // Node 0 gathers all the the data and the broadcast it to all procs.
  if (myId == 0)
    {
    this->RootExecute(com, reader, writer);
    }
  else
    {
    this->SateliteExecute(com, reader, writer);
    }

  reader->Delete();
  reader = NULL;
  writer->Delete();
  writer = NULL;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::RootExecute(vtkMPICommunicator* com,
                                       vtkPolyDataReader* reader,
                                         vtkPolyDataWriter* writer)
{
  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();
  vtkPolyData* input;
  vtkPolyData* output;
  vtkPolyData* pd;

  // Compute the total buffer size needed
  int mySize = 0;
  int sumSize;
  com->ReduceSum(&mySize, &sumSize, 1, 0);

  // Allocate the buffers for gather.
  char *myBuf = NULL;
  char *recvBuf;
  int  *recvLengths;
  int  *recvOffsets;
  recvBuf = new char [sumSize];
  recvLengths = new int [numProcs];
  recvOffsets = new int [numProcs];

  com->GatherV(myBuf, recvBuf, mySize, recvLengths, recvOffsets, 0);

  // Reconstruct the poly datas and append them together.
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  // First append the input from this process.
  input = this->GetInput();
  pd = vtkPolyData::New();
  pd->CopyStructure(input);
  pd->GetPointData()->PassData(input->GetPointData());
  pd->GetCellData()->PassData(input->GetCellData());
  append->AddInput(pd);
  pd->Delete();
  for (int idx = 1; idx < numProcs; ++idx)
    {
    // vtkCharArray should not delete the string when it's done.
    reader->GetInputArray()->SetArray(recvBuf+recvOffsets[idx],
                                      recvLengths[idx], 1);
    output = reader->GetOutput();
    output->Update();
    pd = vtkPolyData::New();
    pd->CopyStructure(output);
    pd->GetPointData()->PassData(output->GetPointData());
    pd->GetCellData()->PassData(output->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    }
  delete [] recvBuf;
  recvBuf = NULL;
  delete [] recvLengths;
  recvLengths = NULL;
  delete [] recvOffsets;
  recvOffsets = NULL;

  // Append
  output = append->GetOutput();
  output->Update();

  // Copy results to our output;
  pd = this->GetOutput();
  pd->CopyStructure(output);
  pd->GetPointData()->PassData(output->GetPointData());
  pd->GetCellData()->PassData(output->GetCellData());

  // Marshal the output.
  writer->SetInput(output);
  writer->Write();
  int size = writer->GetOutputStringLength();
  char* buf = writer->RegisterAndGetOutputString();

  // Broadcast the result to all satelites.
  com->Broadcast(&size, 1, 0);
  com->Broadcast(buf, size, 0);

  // Send the result to the client.
  if (this->SocketController && ! this->ClientFlag)
    {
    this->SocketController->Send(&size, 1, 1, 199292);
    this->SocketController->Send(buf, size, 1, 199293);
    }

  delete [] buf;
}



//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::SateliteExecute(vtkMPICommunicator* com,
                                           vtkPolyDataReader* reader,
                                           vtkPolyDataWriter* writer)
{
  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();
  vtkPolyData* input;
  vtkPolyData* output;
  vtkPolyData* pd;

  // First marshal our input.
  input = this->GetInput();
  pd = vtkPolyData::New();
  pd->CopyStructure(input);
  pd->GetPointData()->PassData(input->GetPointData());
  pd->GetCellData()->PassData(input->GetCellData());
  writer->SetInput(pd);
  writer->Write();
  int size = writer->GetOutputStringLength();
  char* buf = writer->RegisterAndGetOutputString();

  // Compute the total buffer size needed (send size to 0).
  int sumSize; // ignored
  com->ReduceSum(&size, &sumSize, 1, 0);

  // Buffers for gather. (Ignored).
  char *recvBuf = NULL;
  int  *recvLengths = NULL;
  int  *recvOffsets = NULL;
  // Gather sends buffer to node 0.
  com->GatherV(buf, recvBuf, size, recvLengths, recvOffsets, 0);
  delete [] buf;
  buf = NULL;

  // Receive a broadcast (appended model) from node 0.
  com->Broadcast(&size, 1, 0);
  buf = new char [size];
  com->Broadcast(buf, size, 0);

  // Read the string to get the final poly data.
  // vtkCharArray should not delete the string when it's done.
  reader->GetInputArray()->SetArray(buf, size, 1);
  output = reader->GetOutput();
  output->Update();
  pd = this->GetOutput();
  pd->CopyStructure(output);
  pd->GetPointData()->PassData(output->GetPointData());
  pd->GetCellData()->PassData(output->GetCellData());
  delete [] buf;
  buf = NULL;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ClientExecute(vtkPolyDataReader* reader)
{
  int size;
  char* buf;
  vtkPolyData* output;
  vtkPolyData* pd;

  this->SocketController->Receive(&size, 1, 1, 18732);
  buf = new char [size];
  this->SocketController->Receive(buf, size, 1, 18733);
  
  // Read the string to get the final poly data.
  // vtkCharArray should not delete the string when it's done.
  reader->GetInputArray()->SetArray(buf, size, 1);
  output = reader->GetOutput();
  output->Update();
  pd = this->GetOutput();
  pd->CopyStructure(output);
  pd->GetPointData()->PassData(output->GetPointData());
  pd->GetCellData()->PassData(output->GetCellData());
  delete [] buf;
  buf = NULL;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller: (" << this->Controller << ")\n";
  if (this->SocketController)
    {
    os << indent << "SocketController: (" << this->SocketController << ")\n";
    os << indent << "ClientFlag: " << this->ClientFlag << endl;
    }
}

