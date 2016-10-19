/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFileSeriesWriter.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVTrivialProducer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

#include <string>

vtkStandardNewMacro(vtkFileSeriesWriter);
vtkCxxSetObjectMacro(vtkFileSeriesWriter, Writer, vtkAlgorithm);
//-----------------------------------------------------------------------------
vtkFileSeriesWriter::vtkFileSeriesWriter()
{
  this->SetNumberOfOutputPorts(0);

  this->Writer = 0;

  this->FileNameMethod = 0;
  this->FileName = 0;

  this->WriteAllTimeSteps = 0;
  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->Interpreter = 0;
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//-----------------------------------------------------------------------------
vtkFileSeriesWriter::~vtkFileSeriesWriter()
{
  this->SetWriter(0);
  this->SetFileNameMethod(0);
  this->SetFileName(0);
  this->SetInterpreter(0);
}

//----------------------------------------------------------------------------
int vtkFileSeriesWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    vtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();
  if (this->Writer)
  {
    this->Writer->Modified();
  }

  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int vtkFileSeriesWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()) ||
    request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    // Let the internal writer handle the request. Then the request will be
    // "tweaked" by this class.
    if (this->Writer && !this->Writer->ProcessRequest(request, inputVector, outputVector))
    {
      return 0;
    }
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkFileSeriesWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // Does the input have timesteps?
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFileSeriesWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{

  // Piece request etc. has already been set by this->CallWriter(), just set the
  // time request if needed.
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
int vtkFileSeriesWriter::RequestData(vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // this->Writer has already written out the file, just manage the looping for
  // timesteps.

  if (this->CurrentTimeIndex == 0 && this->WriteAllTimeSteps)
  {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  this->WriteATimestep(input, inInfo);

  if (this->WriteAllTimeSteps)
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
void vtkFileSeriesWriter::WriteATimestep(vtkDataObject* input, vtkInformation* inInfo)
{
  std::ostringstream fname;
  if (this->WriteAllTimeSteps && this->NumberOfTimeSteps > 1)
  {
    std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    fname << path << "/" << fnamenoext << "_" << this->CurrentTimeIndex << ext;
  }
  else
  {
    fname << this->FileName;
  }

  // I am guessing we can directly pass the input here (no need to shallow
  // copy), however just to be on safer side, I am creating a shallow copy.
  vtkSmartPointer<vtkDataObject> clone;
  clone.TakeReference(input->NewInstance());
  clone->ShallowCopy(input);

  vtkPVTrivialProducer* tp = vtkPVTrivialProducer::New();
  tp->SetOutput(clone);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    int wholeExtent[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    tp->SetWholeExtent(wholeExtent);
  }
  this->Writer->SetInputConnection(tp->GetOutputPort());
  tp->FastDelete();
  this->SetWriterFileName(fname.str().c_str());
  this->WriteInternal();
  this->Writer->SetInputConnection(0);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is
// modified, then this object is modified as well.
vtkMTimeType vtkFileSeriesWriter::GetMTime()
{
  return this->Superclass::GetMTime();
  /*
  vtkMTimeType mTime=this->vtkObject::GetMTime();
  vtkMTimeType readerMTime;

  if ( this->Writer )
    {
    readerMTime = this->Writer->GetMTime();
    mTime = ( readerMTime > mTime ? readerMTime : mTime );
    }

  return mTime;
  */
}

//-----------------------------------------------------------------------------
void vtkFileSeriesWriter::WriteInternal()
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
void vtkFileSeriesWriter::SetWriterFileName(const char* fname)
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
void vtkFileSeriesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
