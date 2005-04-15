/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIMoveData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIMoveData.h"

#include "vtkAppendPolyData.h"
#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkDataSetWriter.h"
#include "vtkDataSetReader.h"
#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#include "vtkAllToNRedistributePolyData.h"
#endif
#include "vtkMPIMToNSocketConnection.h"
#include "vtkSocketCommunicator.h"

vtkCxxRevisionMacro(vtkMPIMoveData, "1.7");
vtkStandardNewMacro(vtkMPIMoveData);

vtkCxxSetObjectMacro(vtkMPIMoveData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMoveData,ClientDataServerSocketController, vtkSocketController);
vtkCxxSetObjectMacro(vtkMPIMoveData,MPIMToNSocketConnection, vtkMPIMToNSocketConnection);

//-----------------------------------------------------------------------------
vtkMPIMoveData::vtkMPIMoveData()
{
  // Client has no inputs.
  this->NumberOfRequiredInputs = 0;

  this->Controller = 0;
  this->ClientDataServerSocketController = 0;
  this->MPIMToNSocketConnection = 0;

  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->MoveMode = 0;/*vtkMPIMoveData::PASS_THROUGH;*/
  this->DefineCollectAsClone = 0;
  // This tells which server/client this object is on.
  this->Server = -1;

  // This is set on the data server and render server when we are running
  // with a render server.
  this->MPIMToNSocketConnection = 0;

  this->NumberOfBuffers = 0;
  this->BufferLengths = 0;
  this->BufferOffsets = 0;
  this->Buffers = 0;
  this->BufferTotalLength = 9;
}

//-----------------------------------------------------------------------------
vtkMPIMoveData::~vtkMPIMoveData()
{
  this->SetController(0);
  this->SetClientDataServerSocketController(0);
  this->SetMPIMToNSocketConnection(0);
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
int vtkMPIMoveData::CreateOutput(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  this->Superclass::CreateOutput(request, inputVector, outputVector);
  return 1;
}

//-----------------------------------------------------------------------------
// Since this data set to data set fitler may not have an input,
// We need to circumvent the check by the superclass.
vtkDataSet* vtkMPIMoveData::GetOutput()
{
  if (this->NumberOfOutputs > 0)
    {
    return static_cast<vtkDataSet*>(this->Outputs[0]);
    }
  return this->Superclass::GetOutput();
}

//-----------------------------------------------------------------------------
vtkPolyData* vtkMPIMoveData::GetPolyDataOutput()
{
  vtkPolyData* pd;
  if (this->NumberOfOutputs == 0 || this->Outputs[0] == 0)
    {
    pd = vtkPolyData::New();
    this->SetOutput(pd);
    pd->Delete();
    return pd;
    }
  pd = vtkPolyData::SafeDownCast(this->Outputs[0]);
  if (pd == 0)
    {
    vtkErrorMacro("Could not get the poly data output.");
    }
  return pd;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkMPIMoveData::GetUnstructuredGridOutput()
{
  vtkUnstructuredGrid* ug;
  if (this->NumberOfOutputs == 0 || this->Outputs[0] == 0)
    {
    ug = vtkUnstructuredGrid::New();
    this->SetOutput(ug);
    ug->Delete();
    return ug;
    }
    
  ug = vtkUnstructuredGrid::SafeDownCast(this->Outputs[0]);
  if (ug == 0)
    {
    vtkErrorMacro("Could not get the unstructured grid output.");
    }
  return ug;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
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

// This filter  is going to replace the many variations of collection fitlers.
// It handles collection and duplication.
// It handles poly data and unstructured grid.
// It handles rendering on the data server and render server.


// Pass through, No render server. (Distributed rendering on data server).
// Data server copy input to output.

// Passthrough, Yes RenderServer (Distributed rendering on render server).
// Data server MtoN
// Move data from N data server processes to N render server processes.

// Duplicate, No render server. (Tile rendering on data server and client).
// GatherAll on data server.
// Data server process 0 sends data to client.

// Duplicate, Yes RenderServer (Tile rendering on rendering server and client).
// GatherToZero on data server.
// Data server process 0 sends to client
// Data server process 0 sends to render server 0
// Render server process 0 broad casts to all render server processes.

// Collect, render server: yes or no. (client rendering).
// GatherToZero on data server.
// Data server process 0 sends data to client.

//-----------------------------------------------------------------------------
// We should avoid marshalling more than once.
void vtkMPIMoveData::Execute()
{
  vtkDataSet* input = this->GetInput();
  vtkDataSet* output = this->GetOutput();

  // A hack to implement an onld API.
  if (this->DefineCollectAsClone && this->MoveMode == vtkMPIMoveData::COLLECT)
    {
    this->MoveMode = vtkMPIMoveData::CLONE;
    }

  // This case deals with everything running as one MPI group
  // Client, Data and render server are all the same program.
  // This covers single process mode too, although this filter 
  // is unnecessary in that mode and really should not be put in.
  if (this->MPIMToNSocketConnection == 0 &&
      this->ClientDataServerSocketController == 0)
    {
    // Clone in this mode is used for plots and picking.
    if (this->MoveMode == vtkMPIMoveData::CLONE)
      {
      this->DataServerGatherAll(input, output);
      return;
      }
    // Collect mode for rendering on node 0.
    if (this->MoveMode == vtkMPIMoveData::COLLECT)
      {
      this->DataServerGatherToZero(input, output);
      return;
      }
    // PassThrough mode for compositing.
    if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH)
      {
      output->ShallowCopy(input);
      return;
      }
    vtkErrorMacro("MoveMode not set.");
    return;
    }    

  // PassThrough with no RenderServer. (Distributed rendering on data server).
  // Data server copy input to output. 
  // Render server and client will not have an input.
  if (this->MoveMode == 0/*vtkMPIMoveData::PASS_THROUGH*/ && 
      this->MPIMToNSocketConnection == 0)
    {
    if (input)
      {
      output->ShallowCopy(input);
      }
    return;
    }

  // Passthrough and RenderServer (Distributed rendering on render server).
  // Data server MtoN
  // Move data from N data server processes to N render server processes.
  if (this->MoveMode == 0/*vtkMPIMoveData::PASS_THROUGH*/ && 
      this->MPIMToNSocketConnection)
    {
    if (this->Server == 1/*vtkMPIMoveData::DATA_SERVER*/)
      {
      this->DataServerAllToN(input,output, 
                      this->MPIMToNSocketConnection->GetNumberOfConnections());
      this->DataServerSendToRenderServer(output);
      output->Initialize();
      return;
      }
    if (this->Server == 2/*vtkMPIMoveData::RENDER_SERVER*/)
      {
      this->RenderServerReceiveFromDataServer(output);
      return;
      }
    // Client does nothing.
    return;
    }

  // Duplicate with no RenderServer.(Tile rendering on data server and client).
  // GatherAll on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == 2/*vtkMPIMoveData::DUPLICATE*/ && 
      this->MPIMToNSocketConnection ==0)
    {
    if (this->Server == 1/*vtkMPIMoveData::DATA_SERVER*/)
      {
      this->DataServerGatherAll(input, output);
      this->DataServerSendToClient(output);
      return;
      }
    if (this->Server == 0/*vtkMPIMoveData::CLIENT*/)
      {
      this->ClientReceiveFromDataServer(output);
      return;
      }
    }

  // Duplicate and RenderServer(Tile rendering on rendering server and client).
  // GatherToZero on data server.
  // Data server process 0 sends to client
  // Data server process 0 sends to render server 0
  // Render server process 0 broad casts to all render server processes.
  if (this->MoveMode == 2/*vtkMPIMoveData::DUPLICATE*/ && 
      this->MPIMToNSocketConnection)
    {
    if (this->Server == 1/*vtkMPIMoveData::DATA_SERVER*/)
      {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      this->DataServerZeroSendToRenderServerZero(output);
      return;
      }
    if (this->Server == 0/*vtkMPIMoveData::CLIENT*/)
      {
      this->ClientReceiveFromDataServer(output);
      return;
      }
    if (this->Server == 2/*vtkMPIMoveData::RENDER_SERVER*/)
      {
      this->RenderServerZeroReceiveFromDataServerZero(output);
      this->RenderServerZeroBroadcast(output);
      }
    }
  
  // Collect and data server or render server (client rendering).
  // GatherToZero on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == 1/*vtkMPIMoveData::COLLECT*/)
    {
    if (this->Server == 1/*vtkMPIMoveData::DATA_SERVER*/)
      {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      return;
      }
    if (this->Server == 0/*vtkMPIMoveData::CLIENT*/)
      {
      this->ClientReceiveFromDataServer(output);
      return;
      }
    // Render server does nothing
    return;
    }
}

//-----------------------------------------------------------------------------
// Use LANL filter to redistribute the data.
// We will marshal more than once, but that is OK.
void vtkMPIMoveData::DataServerAllToN(vtkDataSet* inData, 
                                      vtkDataSet* outData, int n)
{
  vtkMultiProcessController* controller = this->Controller;
  vtkPolyData* input = vtkPolyData::SafeDownCast(inData);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outData);
  int m;

  if (controller == 0)
    {
    vtkErrorMacro("Missing controller.");
    return;
    }

  m = this->Controller->GetNumberOfProcesses();
  if (n > m)
    {
    vtkWarningMacro("Too many render servers.");
    n = m;
    }
  if (n == m)
    {
    output->ShallowCopy(input);
    }

  if (input == 0 || output == 0)
    {
    vtkErrorMacro("All to N only works for poly data currently.");
    return;
    }

  // Perform the M to N operation.
#ifdef VTK_USE_MPI
  vtkPolyData* tmp;
   vtkAllToNRedistributePolyData* AllToN = NULL;
   vtkPolyData* inputCopy = vtkPolyData::New();
   inputCopy->ShallowCopy(input);
   AllToN = vtkAllToNRedistributePolyData::New();
   AllToN->SetController(controller);
   AllToN->SetNumberOfProcesses(n);
   AllToN->SetInput(inputCopy);
   inputCopy->Delete();
   tmp = AllToN->GetOutput();
   tmp->SetUpdateNumberOfPieces(this->GetOutput()->GetUpdateNumberOfPieces());
   tmp->SetUpdatePiece(this->GetOutput()->GetUpdatePiece());
   tmp->Update();
   output->ShallowCopy(tmp);
   AllToN->Delete();
   AllToN= 0;
#endif
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherAll(vtkDataSet* input, 
                                         vtkDataSet* output)
{
  int numProcs= this->Controller->GetNumberOfProcesses();

  if (numProcs <= 1)
    {
    output->ShallowCopy(input);
    return;
    }

#ifdef VTK_USE_MPI
  int idx;
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  int inBufferLength = this->BufferTotalLength;
  char *inBuffer = this->Buffers;
  this->Buffers = NULL;
  this->ClearBuffer();

  // Allocate arrays used by the AllGatherV call.
  this->BufferLengths = new int[numProcs];
  this->BufferOffsets = new int[numProcs];
  
  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to all other processes.
  com->AllGather(&inBufferLength, this->BufferLengths, 1);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  // Gather the marshaled data sets from all procs.
  this->NumberOfBuffers = numProcs;
  this->Buffers = new char[this->BufferTotalLength];
  com->AllGatherV(inBuffer, this->Buffers, inBufferLength, 
                  this->BufferLengths, this->BufferOffsets);

  this->ReconstructDataFromBuffer(output);
  
  //int fixme; // Do not clear buffers here
  this->ClearBuffer();
#endif
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherToZero(vtkDataSet* input, 
                                            vtkDataSet* output)
{
  int numProcs= this->Controller->GetNumberOfProcesses();
  if (numProcs == 1)
    {
    output->ShallowCopy(input);
    return;
    }

#ifdef VTK_USE_MPI
  int idx;
  int myId= this->Controller->GetLocalProcessId();
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  int inBufferLength = this->BufferTotalLength;
  char *inBuffer = this->Buffers;
  this->Buffers = NULL;
  this->ClearBuffer();

  if (myId == 0)
    {
    // Allocate arrays used by the AllGatherV call.
    this->BufferLengths = new int[numProcs];
    this->BufferOffsets = new int[numProcs];
    }

  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to process 0.
  com->Gather(&inBufferLength, this->BufferLengths, 1, 0);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  if (myId == 0)
    {
    for (idx = 0; idx < numProcs; ++idx)
      {
      this->BufferOffsets[idx] = this->BufferTotalLength;
      this->BufferTotalLength += this->BufferLengths[idx];
      }
    // Gather the marshaled data sets to 0.
    this->Buffers = new char[this->BufferTotalLength];
    }
  com->GatherV(inBuffer, this->Buffers, inBufferLength, 
                  this->BufferLengths, this->BufferOffsets, 0);
  this->NumberOfBuffers = numProcs;

  if (myId == 0)
    {
    this->ReconstructDataFromBuffer(output);
    }

  //int fixme; // Do not clear buffers here
  this->ClearBuffer();

  delete [] inBuffer;
  inBuffer = NULL;
#endif
}




//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToRenderServer(vtkDataSet* output)
{
  vtkSocketCommunicator* com = 
      this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == 0)
    {
    // Some data server may not have sockets because there are more data
    // processes than render server processes.
    return;
    }

  //int fixme;
  // We might be able to eliminate this marshal.
  this->ClearBuffer();
  this->MarshalDataToBuffer(output);

  com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
  com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerReceiveFromDataServer(vtkDataSet* output)
{
  vtkSocketCommunicator* com = 
      this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == 0)
    {
    vtkErrorMacro("All render server processes should have sockets.");
    return;
    }

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
  this->BufferLengths = new int[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  // Compute additional buffer information.
  this->BufferOffsets = new int[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

  //int fixme;  // Can we avoid this?
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerZeroSendToRenderServerZero(vtkDataSet* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    vtkSocketCommunicator* com = 
        this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == 0)
      {
      // Proc 0 (at least) should have a communicator.
      vtkErrorMacro("Missing socket connection.");
      return;
      }

    //int fixme;
    // We might be able to eliminate this marshal.
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
    com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
    this->ClearBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroReceiveFromDataServerZero(vtkDataSet* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    vtkSocketCommunicator* com = 
        this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == 0)
      {
      vtkErrorMacro("All render server processes should have sockets.");
      return;
      }

    this->ClearBuffer();
    com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
    this->BufferLengths = new int[this->NumberOfBuffers];
    com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    // Compute additional buffer information.
    this->BufferOffsets = new int[this->NumberOfBuffers];
    this->BufferTotalLength = 0;
    for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
      {
      this->BufferOffsets[idx] = this->BufferTotalLength;
      this->BufferTotalLength += this->BufferLengths[idx];
      }
    this->Buffers = new char[this->BufferTotalLength];
    com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

    //int fixme;  // Can we avoid this?
    this->ReconstructDataFromBuffer(data);
    this->ClearBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToClient(vtkDataSet* output)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    this->ClearBuffer();
    this->MarshalDataToBuffer(output);
    this->ClientDataServerSocketController->Send(
                                     &(this->NumberOfBuffers), 1, 1, 23490);
    this->ClientDataServerSocketController->Send(this->BufferLengths, 
                                     this->NumberOfBuffers, 1, 23491);
    this->ClientDataServerSocketController->Send(this->Buffers, 
                                     this->BufferTotalLength, 1, 23492);
    this->ClearBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClientReceiveFromDataServer(vtkDataSet* output)
{
  vtkCommunicator* com = 0;
  com = this->ClientDataServerSocketController->GetCommunicator();
  if (com == 0)
    {
    vtkErrorMacro("Missing socket controler on cleint.");
    return;
    }

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23490);
  this->BufferLengths = new int[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 
                                  1, 23491);
  // Compute additional buffer information.
  this->BufferOffsets = new int[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 
                                  1, 23492);
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}


//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroBroadcast(vtkDataSet* data)
{
  (void)data; // shut up warning
  int numProcs= this->Controller->GetNumberOfProcesses();
  if (numProcs <= 1)
    {
    return;
    }

#ifdef VTK_USE_MPI
  int myId= this->Controller->GetLocalProcessId();

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }

  int bufferLength = 0;
  if (myId == 0)
    {
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    bufferLength = this->BufferLengths[0];
    }
  
  // Broadcast the size of the buffer.
  com->Broadcast(&bufferLength, 1, 0);

  // Allocate buffers for all receiving nodes.
  if (myId != 0)
    {
    this->NumberOfBuffers = 1;
    this->BufferLengths = new int[1];
    this->BufferLengths[0] = bufferLength;
    this->BufferOffsets = new int[1];
    this->BufferOffsets[0] = 0;
    this->BufferTotalLength = this->BufferLengths[0];
    this->Buffers = new char[bufferLength];
    }

  // Broadcast the buffer.
  com->Broadcast(this->Buffers, bufferLength, 0);

  // Reconstruct the output on nodes other than 0.
  if (myId != 0)
    {
    this->ReconstructDataFromBuffer(data);
    }
  
  this->ClearBuffer();
#endif
}




//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClearBuffer()
{
  this->NumberOfBuffers = 0;
  if (this->BufferLengths)
    {
    delete [] this->BufferLengths;
    this->BufferLengths = 0;
    }
  if (this->BufferOffsets)
    {
    delete [] this->BufferOffsets;
    this->BufferOffsets = 0;
    }
  if (this->Buffers)
    {
    delete [] this->Buffers;
    this->Buffers = 0;
    }
  this->BufferTotalLength = 0;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::MarshalDataToBuffer(vtkDataSet* data)
{
  // Protect from empty data.
  if (data->GetNumberOfPoints() == 0)
    {
    this->NumberOfBuffers = 0;
    }

  // Copy input to isolate reader from the pipeline.
  vtkDataSet* d = data->NewInstance();
  d->CopyStructure(data);
  d->GetPointData()->PassData(data->GetPointData());
  d->GetCellData()->PassData(data->GetCellData());
  // Marshal with writer.
  vtkDataSetWriter *writer = vtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(d);
  writer->Write();
  // Get string.
  this->NumberOfBuffers = 1;
  this->BufferLengths = new int[1];
  this->BufferLengths[0] = writer->GetOutputStringLength();
  this->BufferOffsets = new int[1];
  this->BufferOffsets[0] = 0;
  this->BufferTotalLength = this->BufferLengths[0];
  this->Buffers = writer->RegisterAndGetOutputString();

  d->Delete();
  d = 0;
  writer->Delete();
  writer = 0;
}


//-----------------------------------------------------------------------------
void vtkMPIMoveData::ReconstructDataFromBuffer(vtkDataSet* data)
{
  if (this->NumberOfBuffers == 0 || this->Buffers == 0)
    {
    data->Initialize();
    return;
    }

  // PolyData and Unstructured grid need different append filters.
  vtkAppendPolyData* appendPd = NULL;
  vtkAppendFilter*   appendUg = NULL;
  if (this->NumberOfBuffers > 1)
    {
    if (data->IsA("vtkPolyData"))
      {
      appendPd = vtkAppendPolyData::New();
      }
    else if (data->IsA("vtkUnstructuredGrid"))
      {
      appendUg = vtkAppendFilter::New();
      }
    else
      {
      vtkErrorMacro("This filter only handles unstructured data.");
      return;
      }
    }

  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    // Setup a reader.
    vtkDataSetReader *reader = vtkDataSetReader::New();
    reader->ReadFromInputStringOn();
    vtkCharArray* mystring = vtkCharArray::New();
    mystring->SetArray(this->Buffers+this->BufferOffsets[idx], 
                       this->BufferLengths[idx], 1);
    reader->SetInputArray(mystring);
    reader->Modified(); // For append loop
    reader->GetOutput()->Update();
    if (appendPd)
      { 
      appendPd->AddInput(reader->GetPolyDataOutput());
      }
    else if (appendUg)
      { 
      appendUg->AddInput(reader->GetUnstructuredGridOutput());
      }
    else
      {
      vtkDataSet* out = reader->GetOutput();
      data->CopyStructure(out);
      data->GetPointData()->PassData(out->GetPointData());
      data->GetCellData()->PassData(out->GetCellData());
      }
    mystring->Delete();
    mystring = 0;
    reader->Delete();
    reader = NULL;
    }

  if (appendPd)
    {
    vtkDataSet* out = appendPd->GetOutput();
    out->Update();
    data->CopyStructure(out);
    data->GetPointData()->PassData(out->GetPointData());
    data->GetCellData()->PassData(out->GetCellData());
    appendPd->Delete();
    appendPd = NULL;
    }
  if (appendUg)
    {
    vtkDataSet* out = appendUg->GetOutput();
    out->Update();
    data->CopyStructure(out);
    data->GetPointData()->PassData(out->GetPointData());
    data->GetCellData()->PassData(out->GetCellData());
    appendUg->Delete();
    appendUg = NULL;
    }
}





//-----------------------------------------------------------------------------
void vtkMPIMoveData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfBuffers: " << this->NumberOfBuffers << endl;
  os << indent << "DefineCollectAsClone: " 
     << this->DefineCollectAsClone << endl;
  os << indent << "Server: " << this->Server << endl;
  os << indent << "MoveMode: " << this->MoveMode << endl;

  //os << indent << "MToN
}

