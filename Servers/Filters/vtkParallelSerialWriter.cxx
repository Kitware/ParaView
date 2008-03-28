/*=========================================================================

  Program:   ParaView
  Module:    vtkParallelSerialWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelSerialWriter.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkParallelSerialWriter);
vtkCxxRevisionMacro(vtkParallelSerialWriter, "1.2");

vtkCxxSetObjectMacro(vtkParallelSerialWriter,Writer,vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkParallelSerialWriter::vtkParallelSerialWriter()
{
  this->SetNumberOfOutputPorts(0);

  this->Writer = 0;

  this->FileNameMethod = 0;
  this->FileName = 0;

  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
}

//-----------------------------------------------------------------------------
vtkParallelSerialWriter::~vtkParallelSerialWriter()
{
  if (this->Writer)
    {
    this->Writer->Delete();
    }
  this->SetFileNameMethod(0);
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }

  // always write even if the data hasn't changed
  this->Modified();

  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 
    this->NumberOfPieces);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 
    this->GhostLevel);
  return 1;  
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkSmartPointer<vtkDataObject> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->ShallowCopy(input);

  if (!this->Writer)
    {
    vtkErrorMacro("No internal writer specified. Cannot write.");
    return 0;
    }
  vtkMultiProcessController* controller = 
    vtkProcessModule::GetProcessModule()->GetController();

  vtkSmartPointer<vtkMPIMoveData> md = vtkSmartPointer<vtkMPIMoveData>::New();
  md->SetOutputDataType(VTK_POLY_DATA);
  md->SetController(controller);
  md->SetMoveModeToCollect();
  md->SetInputConnection(0, inputCopy->GetProducerPort());
  md->UpdateInformation();
  vtkInformation* outInfo = md->GetExecutive()->GetOutputInformation(0);
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    this->Piece);
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    this->NumberOfPieces);
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    this->GhostLevel);
  md->Update();

  vtkDataObject* output = md->GetOutputDataObject(0);
  vtkSmartPointer<vtkDataObject> outputCopy;
  outputCopy.TakeReference(output->NewInstance());
  outputCopy->ShallowCopy(output);
  
  if (controller->GetLocalProcessId() == 0)
    {
    this->Writer->SetInputConnection(outputCopy->GetProducerPort());
    this->SetWriterFileName();
    this->WriteInternal();
    }

  return 1;
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is 
// modified, then this object is modified as well.
unsigned long vtkParallelSerialWriter::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long readerMTime;

  if ( this->Writer )
    {
    readerMTime = this->Writer->GetMTime();
    mTime = ( readerMTime > mTime ? readerMTime : mTime );
    }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteInternal()
{
  if (this->Writer)
    {
    vtkClientServerID csId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(this->Writer);
    if (csId.ID && this->FileNameMethod)
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      // Get the local process interpreter.
      vtkClientServerInterpreter* interp = pm->GetInterpreter();
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << csId << "Write"
             << vtkClientServerStream::End;
      interp->ProcessStream(stream);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::SetWriterFileName()
{
  if (this->Writer && this->FileName)
    {
    vtkClientServerID csId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(this->Writer);
    if (csId.ID && this->FileNameMethod)
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      // Get the local process interpreter.
      vtkClientServerInterpreter* interp = pm->GetInterpreter();
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << csId << this->FileNameMethod << this->FileName
             << vtkClientServerStream::End;
      interp->ProcessStream(stream);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
