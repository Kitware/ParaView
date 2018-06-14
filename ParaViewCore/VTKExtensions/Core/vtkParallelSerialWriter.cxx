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
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFileSeriesWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

namespace
{
bool vtkIsEmpty(vtkDataObject* dobj)
{
  for (int cc = 0; (dobj != NULL) && (cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS); ++cc)
  {
    if (dobj->GetNumberOfElements(cc) > 0)
    {
      return false;
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkParallelSerialWriter);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, Writer, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, PreGatherHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, PostGatherHelper, vtkAlgorithm);
//-----------------------------------------------------------------------------
vtkParallelSerialWriter::vtkParallelSerialWriter()
{
  this->SetNumberOfOutputPorts(0);

  this->Writer = nullptr;

  this->FileNameMethod = nullptr;
  this->FileName = nullptr;
  this->FileNameSuffix = nullptr;

  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->PreGatherHelper = nullptr;
  this->PostGatherHelper = nullptr;

  this->WriteAllTimeSteps = 0;
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;

  this->Interpreter = nullptr;
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//-----------------------------------------------------------------------------
vtkParallelSerialWriter::~vtkParallelSerialWriter()
{
  this->SetWriter(nullptr);
  this->SetFileNameMethod(nullptr);
  this->SetFileName(nullptr);
  this->SetFileNameSuffix(nullptr);
  this->SetPreGatherHelper(nullptr);
  this->SetPostGatherHelper(nullptr);
  this->SetInterpreter(nullptr);
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
int vtkParallelSerialWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);

  double* inTimes =
    inputVector[0]->GetInformationObject(0)->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
  {
    double timeReq = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Writer)
  {
    vtkErrorMacro("No internal writer specified. Cannot write.");
    return 0;
  }

  bool write_all = (this->WriteAllTimeSteps != 0 && this->NumberOfTimeSteps > 0);

  if (write_all)
  {
    if (this->CurrentTimeIndex == 0)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  }
  else
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  this->WriteATimestep(input);

  if (write_all)
  {
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteATimestep(vtkDataObject* input)
{
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(input);
  if (cds)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());
    iter->SetSkipEmptyNodes(0);
    int idx;
    for (idx = 0, iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), idx++)
    {
      vtkDataObject* curObj = iter->GetCurrentDataObject();
      std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
      std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
      std::string ext = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
      std::ostringstream fname;
      fname << path << "/" << fnamenoext << idx << ext;
      this->WriteAFile(fname.str().c_str(), curObj);
    }
  }
  else if (input)
  {
    vtkSmartPointer<vtkDataObject> inputCopy;
    inputCopy.TakeReference(input->NewInstance());
    inputCopy->ShallowCopy(input);
    this->WriteAFile(this->FileName, inputCopy);
  }
}

//----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteAFile(const char* filename, vtkDataObject* input)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  vtkSmartPointer<vtkReductionFilter> reductionFilter = vtkSmartPointer<vtkReductionFilter>::New();
  reductionFilter->SetController(controller);
  reductionFilter->SetPreGatherHelper(this->PreGatherHelper);
  reductionFilter->SetPostGatherHelper(this->PostGatherHelper);
  reductionFilter->SetInputDataObject(input);
  reductionFilter->UpdateInformation();
  vtkInformation* outInfo = reductionFilter->GetExecutive()->GetOutputInformation(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  reductionFilter->Update();

  if (controller->GetLocalProcessId() == 0)
  {
    vtkDataObject* output = reductionFilter->GetOutputDataObject(0);
    if (vtkIsEmpty(output) == false)
    {
      std::ostringstream fname;
      if (this->WriteAllTimeSteps)
      {
        std::string path = vtksys::SystemTools::GetFilenamePath(filename);
        std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
        std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
        if (this->FileNameSuffix && vtkFileSeriesWriter::SuffixValidation(this->FileNameSuffix))
        {
          // Print this->CurrentTimeIndex to a string using this->FileNameSuffix as format
          char suffix[100];
          snprintf(suffix, 100, this->FileNameSuffix, this->CurrentTimeIndex);
          fname << path << "/" << fnamenoext << suffix << ext;
        }
        else
        {
          fname << path << "/" << fnamenoext << "." << this->CurrentTimeIndex << ext;
        }
      }
      else
      {
        fname << filename;
      }
      this->Writer->SetInputDataObject(output);
      this->SetWriterFileName(fname.str().c_str());
      this->WriteInternal();
      this->Writer->SetInputConnection(0);
    }
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is
// modified, then this object is modified as well.
vtkMTimeType vtkParallelSerialWriter::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType readerMTime;

  if (this->Writer)
  {
    readerMTime = this->Writer->GetMTime();
    mTime = (readerMTime > mTime ? readerMTime : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteInternal()
{
  if (this->Writer && this->FileNameMethod)
  {
    // Get the local process interpreter.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << "Write"
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::SetWriterFileName(const char* fname)
{
  if (this->Writer && this->FileName && this->FileNameMethod)
  {
    // Get the local process interpreter.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << this->FileNameMethod << fname
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
