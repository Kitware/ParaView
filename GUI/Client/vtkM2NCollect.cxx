/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkM2NCollect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkM2NCollect.h"

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
#endif

vtkCxxRevisionMacro(vtkM2NCollect, "1.1");
vtkStandardNewMacro(vtkM2NCollect);

//vtkCxxSetObjectMacro(vtkM2NCollect,Controller, vtkMultiProcessController);
//vtkCxxSetObjectMacro(vtkM2NCollect,SocketController, vtkSocketController);

//-----------------------------------------------------------------------------
vtkM2NCollect::vtkM2NCollect()
{
  // Client has no inputs.
  this->NumberOfRequiredInputs = 0;
  this->ServerMode =0;
  this->RenderServerMode =0;
  this->ClientMode = 0;
}

//-----------------------------------------------------------------------------
vtkM2NCollect::~vtkM2NCollect()
{
}


//-----------------------------------------------------------------------------
void vtkM2NCollect::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkM2NCollect::ComputeInputUpdateExtents(vtkDataObject *output)
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
void vtkM2NCollect::Execute()
{
  vtkPolyData *input = this->GetInput();
  int bufSize = 0;
  char* buf = NULL;


  // If we have data, marshal it.
  if (input && input->GetNumberOfPoints() > 0)
    {
    // Copy input to isolate reader froim the pipeline.
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
    // Get rid of writer and copy of input.
    writer->Delete();
    writer = NULL;
    pd->Delete();
    pd = NULL;
    }

  // M to N stuff.
  // Communicate sizes (future: array of sizes).
  char* newBuf = NULL;
  int newBufSize = this->ShuffleSizes(bufSize);
  if (newBufSize > 0)
    {
    newBuf = new char[newBufSize];
    }
  // Shuffle memory.
  this->Shuffle(bufSize, buf, newBufSize, newBuf);
  if (buf)
    {
    delete [] newBuf;
    }
  
  // Create the output.
  if (newBuf)
    {
    // Setup a reader.
    vtkPolyDataReader *reader = vtkPolyDataReader::New();
    reader->ReadFromInputStringOn();
    vtkCharArray* mystring = vtkCharArray::New();
    mystring->SetArray(newBuf, newBufSize, 1);
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
    delete [] newBuf;
    newBuf = NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkM2NCollect::ShuffleSizes(int size)
{
 //  if(server)
//     {
//     rsSocket->Send(size);
//     return 0;
//     }
//   if(renderServer)
//     {
//     rsSocket->Receive(val);
//     return val;
//     }
//   return 0;
//   // if I am a server process size will have a non-zero value
//   // need to send that to the render server process if there is one
//   // render server process returns the one it gets from the receive
  return 0;
}

//-----------------------------------------------------------------------------
void vtkM2NCollect::Shuffle(int inSize, char* inBuf, int outSize, char* outBuf)
{
//   if(server)
//     {
//     rsSocket->Send(inBuf, inSize);
//     return;
//     }
//   if(renderServer)
//     {
//     rsSocket->Receive(outSize, outBuf);
//     return;
//     }
}

//-----------------------------------------------------------------------------
void vtkM2NCollect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

