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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

#include <vtkstd/string>

vtkStandardNewMacro(vtkParallelSerialWriter);
vtkCxxRevisionMacro(vtkParallelSerialWriter, "1.4");

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
if (!this->Writer)
  {
  vtkErrorMacro("No internal writer specified. Cannot write.");
  return 0;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkPolyData* pdInput = vtkPolyData::SafeDownCast(input);
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(input);
  if (pdInput)
    {
    vtkSmartPointer<vtkPolyData> inputCopy;
    inputCopy.TakeReference(pdInput->NewInstance());
    inputCopy->ShallowCopy(pdInput);
  
    this->WriteAFile(this->FileName, inputCopy);
    }
  else if (cds)
    {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());
    iter->SetSkipEmptyNodes(0);
    int idx;
    for(idx=0, iter->InitTraversal(); 
        !iter->IsDoneWithTraversal(); 
        iter->GoToNextItem(), idx++)
      {
      vtkDataObject* curObj = iter->GetCurrentDataObject();
      vtkSmartPointer<vtkPolyData> pd;
      if (curObj)
        {
        pd = vtkPolyData::SafeDownCast(curObj);
        if (!pd.GetPointer())
          {
          vtkErrorMacro("Cannot write data object of type: "
              << curObj->GetClassName());
          }
        }
      if (!pd.GetPointer())
        {
        pd.TakeReference(vtkPolyData::New());
        }
      vtkstd::string path = 
        vtksys::SystemTools::GetFilenamePath(this->FileName);
      vtkstd::string fnamenoext =
        vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
      vtkstd::string ext =
        vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
      vtksys_ios::ostringstream fname;
      fname << path << "/" << fnamenoext << idx << ext;
      
      this->WriteAFile(fname.str().c_str(), pd);
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteAFile(const char* fname,
    vtkPolyData* input)
{
  vtkMultiProcessController* controller = 
    vtkProcessModule::GetProcessModule()->GetController();
  
  vtkSmartPointer<vtkMPIMoveData> md = vtkSmartPointer<vtkMPIMoveData>::New();
  md->SetOutputDataType(VTK_POLY_DATA);
  md->SetController(controller);
  md->SetMoveModeToCollect();
  md->SetInputConnection(0, input->GetProducerPort());
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
    this->SetWriterFileName(fname);
    this->WriteInternal();
    this->Writer->SetInputConnection(0);
    }
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
void vtkParallelSerialWriter::SetWriterFileName(const char* fname)
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
             << csId << this->FileNameMethod << fname
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
