/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIDuplicatePolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

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
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkMPIDuplicatePolyData, "1.1.2.1");
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
  this->PassThrough = 0;
  this->ZeroEmpty = 0;
  //this->MemorySize = 0;
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
  
  vtkTimerLog::MarkStartEvent("MPIGather");
  
  if (input == NULL)
    { // This will probably lock up.
    vtkErrorMacro("Input has not been set.");
    return;
    }

  if (this->PassThrough)
    {
    vtkPolyData* output = this->GetOutput();
    output->CopyStructure(input);
    output->GetPointData()->ShallowCopy(input->GetPointData());
    output->GetCellData()->ShallowCopy(input->GetCellData());
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
  
  vtkMPICommunicator *com = NULL;
  if (this->Controller)
    {
    com = vtkMPICommunicator::SafeDownCast(
      this->Controller->GetCommunicator());
    }
  
  if (com == NULL)
    { // This will probably lock up.
    vtkErrorMacro("Could not get MPI communicator.");
    reader->Delete();
    reader = NULL;
    return;
    }
  
  // Create the writer.
  vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();

  // Node 0 gathers all the the data and the broadcast it to all procs.
  this->ServerExecute(com, reader, writer);

  reader->Delete();
  reader = NULL;
  writer->Delete();
  writer = NULL;

  vtkTimerLog::MarkEndEvent("MPIGather");
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ServerExecute(vtkMPICommunicator* com,
                                            vtkPolyDataReader* reader,
                                            vtkPolyDataWriter* writer)
{
  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();
  vtkPolyData* input;
  vtkPolyData* output;
  vtkPolyData* pd;
  int ret, idx;
  int sum;
  int myId;
  
  // Lets just experiment with the Alltoall call.
  /*
  char* send_buffer = new char[100]; //[1];
  int* send_counts = new int[numProcs];
  int* send_displacements = new int[numProcs];
  char* recv_buffer = new char[100];
  int* recv_counts = new int[numProcs];
  int* recv_displacements = new int[numProcs];
  
  myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    strcpy(send_buffer, "abcdefghijklmnopqrstuvwxyz");
    send_displacements[0] = 0;
    send_counts[0] = 10;
    send_displacements[1] = 0;
    send_counts[1] = 10;
    }
  else
    {
    strcpy(send_buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    send_displacements[0] = 0;
    send_counts[0] = 10;
    send_displacements[1] = 0;
    send_counts[1] = 10;
    }
  
  recv_displacements[0] = 0;
  recv_counts[0] = 10;
  recv_displacements[1] = 10;
  recv_counts[1] = 10;
  
  
  com->AllToAllV(send_buffer, send_counts, send_displacements, 
                 recv_buffer, recv_counts, recv_displacements);  
  */

  // Test code.
  /*
  myId = this->Controller->GetLocalProcessId();
  int size;
  char* buf;
  if (myId == 0)
    {
    size = 10;
    buf = new char[26];    
    strncpy(buf, "abcdefghijklmnopqrstuvwxyz", 26);
    }
  else
    {
    size = 10;
    buf = new char[26];    
    strncpy(buf, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
    }
  */
  
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
  
  // Compute the displacements.
  sum = 0;
  
  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to all other processes.
  int* recvLengths = new int[numProcs];
  com->AllGather(&size, recvLengths, 1);
  int* recvOffsets = new int[numProcs];
  sum = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    recvOffsets[idx] = sum;
    sum += recvLengths[idx];
    }
  
  // Get the marshaled data sets.
  char* allbuffers = new char[sum];
  com->AllGatherV(buf, size, allbuffers, recvLengths, recvOffsets);

  // Reconstruct the poly datas and append them together.
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  // First append the input from this process.
  for (idx = 0; 0, idx < numProcs; ++idx) // SKip for debugging !!!!!!!
    {
    // vtkCharArray should not delete the string when it's done.
    reader->Modified();
    reader->GetInputArray()->SetArray(allbuffers+recvOffsets[idx],
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
  delete [] buf;
  buf = NULL;
  delete [] allbuffers;
  allbuffers = NULL;
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
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ClientExecute(vtkPolyDataReader* reader)
{
  /*
  int size;
  char* buf;
  vtkPolyData* output;
  vtkPolyData* pd;

  this->SocketController->Receive(&size, 1, 1, 199292);
  buf = new char [size];
  this->SocketController->Receive(buf, size, 1, 199293);
  
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
  */
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
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "ZeroEmpty: " << this->ZeroEmpty << endl;
  
}

