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
#include "vtkCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "vtkProcessModule.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

vtkCxxRevisionMacro(vtkMPIDuplicatePolyData, "1.15");
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
  this->SetSocketController(0);
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
int vtkMPIDuplicatePolyData::RequestInformation(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  return 1;
}
  
//-----------------------------------------------------------------------------
int vtkMPIDuplicatePolyData::RequestData(vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkTimerLog::MarkStartEvent("MPIGather");
  
  if (this->PassThrough)
    {
    if (input == NULL)
      {
      return 1;
      }
    output->CopyStructure(input);
    output->GetPointData()->ShallowCopy(input->GetPointData());
    output->GetCellData()->ShallowCopy(input->GetCellData());
    return 1;
    }
  
  // Setup a reader.
  vtkPolyDataReader *reader = vtkPolyDataReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* mystring = vtkCharArray::New();
  reader->SetInputArray(mystring);
  mystring->Delete();

  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute(reader, output);
    reader->Delete();
    reader = NULL;
    return 1;
    }
  
  // Create the writer.
  vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();

  // Node 0 gathers all the the data and the broadcast it to all procs.
  this->ServerExecute(reader, writer, input, output);

  reader->Delete();
  reader = NULL;
  writer->Delete();
  writer = NULL;

  vtkTimerLog::MarkEndEvent("MPIGather");

  return 1;
}


//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkMPIDuplicatePolyData::ServerExecute(vtkPolyDataReader* reader,
                                            vtkPolyDataWriter* writer,
                                            vtkPolyData* input,
                                            vtkPolyData* output)
#else
void vtkMPIDuplicatePolyData::ServerExecute(vtkPolyDataReader* ,
                                            vtkPolyDataWriter* writer,
                                            vtkPolyData* input,
                                            vtkPolyData* output)
#endif
{
  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();
  vtkPolyData* pd;
  
  // First marshal our input.
  pd = vtkPolyData::New();
  if (input)
    {
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    }
  writer->SetInput(pd);
  writer->Write();
  int size = writer->GetOutputStringLength();
  char* buf = writer->RegisterAndGetOutputString();
  
  pd->Delete();
  pd = NULL;
  
#ifdef VTK_USE_MPI
  int idx;
  int sum;
  int myId = this->Controller->GetLocalProcessId();

  vtkMPICommunicator *com = NULL;
  if (this->Controller)
    {
    com = vtkMPICommunicator::SafeDownCast(
      this->Controller->GetCommunicator());
    }

  if (com)
    {
    // Allocate arrays used by the AllGatherV call.
    int* recvLengths = new int[numProcs * 2];
    // Use a single array for lengths and offsets to avoid
    // another send to the client.
    int* recvOffsets = recvLengths+numProcs;
  
    // Compute the degenerate input offsets and lengths.
    // Broadcast our size to all other processes.
    com->AllGather(&size, recvLengths, 1);

    // Compute the displacements.
    sum = 0;
    for (idx = 0; idx < numProcs; ++idx)
      {
      recvOffsets[idx] = sum;
      sum += recvLengths[idx];
      }
  
    // Gather the marshaled data sets from all procs.
    char* allbuffers = new char[sum];
    com->AllGatherV(buf, allbuffers, size, recvLengths, recvOffsets);
    if (myId == 0 && this->SocketController)
      {
      // Send the string to the client.
      this->SocketController->Send(&numProcs, 1, 
                                   1, vtkProcessModule::DuplicatePDNProcs);
      this->SocketController->Send(recvLengths, numProcs*2, 
                                   1, vtkProcessModule::DuplicatePDNRecLen);
      this->SocketController->Send(allbuffers, sum, 
                                   1, vtkProcessModule::DuplicatePDNAllBuffers);
      }
    this->ReconstructOutput(reader, numProcs, allbuffers, 
                            recvLengths, recvOffsets, output);
    delete [] allbuffers;
    allbuffers = NULL;
    delete [] recvLengths;
    recvLengths = NULL;
    // recvOffsets is part of the recvLengths array.
    recvOffsets = NULL;
    delete [] buf;
    buf = NULL;
    return;
    }

#endif

  // Server must be a single process!
  if (this->SocketController)
    {
    this->SocketController->Send(&numProcs, 1, 
                                 1, vtkProcessModule::DuplicatePDNProcs);
    int tmp[2];
    tmp[0] = size;
    tmp[1] = 0;
    this->SocketController->Send(tmp, 2, 
                                 1, vtkProcessModule::DuplicatePDNRecLen);
    this->SocketController->Send(buf, size, 
                                 1, vtkProcessModule::DuplicatePDNAllBuffers);
    }
  // Degenerate reconstruct output.
  if (input)
    {
    output->ShallowCopy(input);
    }
  delete [] buf;
  buf = NULL;
}



//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ReconstructOutput(vtkPolyDataReader* reader,
                                                int numProcs, char* recv,
                                                int* recvLengths, 
                                                int* recvOffsets,
                                                vtkPolyData* pd)
{
  vtkPolyData* output;
  int idx;

  // Reconstruct the poly datas and append them together.
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  // First append the input from this process.
  for (idx = 0; idx < numProcs; ++idx)
    {
    // vtkCharArray should not delete the string when it's done.
    reader->Modified();
    reader->GetInputArray()->SetArray(recv+recvOffsets[idx],
                                      recvLengths[idx], 1);
    output = reader->GetOutput();
    output->Update();
    pd = vtkPolyData::New();
    pd->CopyStructure(output);
    pd->GetPointData()->PassData(output->GetPointData());
    pd->GetCellData()->PassData(output->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    pd = NULL;
    }

  // Append
  output = append->GetOutput();
  output->Update();

  // Copy results to our output;
  pd->CopyStructure(output);
  pd->GetPointData()->PassData(output->GetPointData());
  pd->GetCellData()->PassData(output->GetCellData());

  append->Delete();
  append = NULL;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicatePolyData::ClientExecute(vtkPolyDataReader* reader,
                                            vtkPolyData* output)
{
  int numProcs;
  int* recvLengths;
  int* recvOffsets;
  int totalLength, idx;
  char* buffers;

  // Receive the numer of processes.
  this->SocketController->Receive(&numProcs, 1, 
                                  1, vtkProcessModule::DuplicatePDNProcs);

  // Receive information about the lengths/offsets of each marshaled data set.
  recvLengths = new int[numProcs*2];
  recvOffsets = recvLengths + numProcs;
  this->SocketController->Receive(recvLengths, numProcs*2, 
                                  1, vtkProcessModule::DuplicatePDNRecLen);

  // Receive the actual buffers.
  totalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    totalLength += recvLengths[idx];
    }
  buffers = new char[totalLength];
  this->SocketController->Receive(buffers, totalLength, 
                                  1, vtkProcessModule::DuplicatePDNAllBuffers);

  this->ReconstructOutput(reader, numProcs, buffers, 
                          recvLengths, recvOffsets, output);
  delete [] recvLengths;
  recvLengths = NULL;
  delete [] buffers;
  buffers = NULL;
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
  // this->MemorySize doesn't exist (if vtkCollectUnstructuredGrid API is ever
  // to be mimicked, then this may need to be declared).
}


//----------------------------------------------------------------------------
int vtkMPIDuplicatePolyData::FillInputPortInformation(int , 
                                                      vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}
