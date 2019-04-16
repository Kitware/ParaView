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

  this->Writer = nullptr;

  this->FileNameMethod = nullptr;
  this->FileName = nullptr;
  this->FileNameSuffix = nullptr;

  this->WriteAllTimeSteps = 0;
  this->MinTimeStep = 0;
  this->MaxTimeStep = -1;
  this->TimeStepStride = 1;

  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->Interpreter = nullptr;
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//-----------------------------------------------------------------------------
vtkFileSeriesWriter::~vtkFileSeriesWriter()
{
  this->SetWriter(nullptr);
  this->SetFileNameMethod(nullptr);
  this->SetFileName(nullptr);
  this->SetInterpreter(nullptr);
  this->SetFileNameSuffix(nullptr);
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
    // if the number of time steps is less than the min time step then we just write out the
    // current time step assuming that's better than nothing but we'll give a warning
    if (this->NumberOfTimeSteps < this->MinTimeStep)
    {
      vtkWarningMacro("There are less time steps ("
        << this->NumberOfTimeSteps << ") than the minimum requested time step ("
        << this->MinTimeStep << ") so the current time step will be written out instead.\n");
    }
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
  if (this->NumberOfTimeSteps < this->MinTimeStep)
  {
    return 1;
  }
  double* inTimes =
    inputVector[0]->GetInformationObject(0)->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
  {
    if (this->CurrentTimeIndex < this->MinTimeStep)
    {
      this->CurrentTimeIndex = this->MinTimeStep;
    }
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

  if (this->WriteAllTimeSteps && this->MinTimeStep <= this->NumberOfTimeSteps &&
    (this->CurrentTimeIndex == 0 || (this->CurrentTimeIndex == this->MinTimeStep)))
  {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!this->WriteATimestep(input, inInfo))
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    return 0;
  }

  if (this->WriteAllTimeSteps)
  {
    this->CurrentTimeIndex += this->TimeStepStride;
    if ((this->CurrentTimeIndex >= this->NumberOfTimeSteps) ||
      (this->MaxTimeStep > this->MinTimeStep && this->CurrentTimeIndex > this->MaxTimeStep))
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }
  }

  return 1;
}
//----------------------------------------------------------------------------
bool vtkFileSeriesWriter::WriteATimestep(vtkDataObject* input, vtkInformation* inInfo)
{
  std::ostringstream fname;
  if (this->WriteAllTimeSteps && this->NumberOfTimeSteps > 1)
  {
    std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    if (this->FileNameSuffix && vtkFileSeriesWriter::SuffixValidation(this->FileNameSuffix))
    {
      // Print this->CurrentTimeIndex to a string using this->FileNameSuffix as format
      char suffix[100];
      snprintf(suffix, 100, this->FileNameSuffix, this->CurrentTimeIndex);
      if (!path.empty() && path != "")
      {
        fname << path << "/";
      }
      fname << fnamenoext << suffix << ext;
    }
    else
    {
      vtkErrorMacro("Invalid file suffix:" << (this->FileNameSuffix ? this->FileNameSuffix : "null")
                                           << ". Expected valid % format specifiers!");
      return false;
    }
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

  return true;
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
bool vtkFileSeriesWriter::SuffixValidation(char* fileNameSuffix)
{
  std::string suffix(fileNameSuffix);
  // Only allow this format: ABC%.Xd
  // ABC is an arbitrary string which may or may not exist
  // % and d must exist and d must be the last char
  // . and X may or may not exist, X must be an integer if it exists
  if (suffix.empty() || suffix[suffix.size() - 1] != 'd')
  {
    return false;
  }
  std::string::size_type lastPercentage = suffix.find_last_of('%');
  if (lastPercentage == std::string::npos)
  {
    return false;
  }
  if (suffix.size() - lastPercentage > 2 && !isdigit(suffix[lastPercentage + 1]) &&
    suffix[lastPercentage + 1] != '.')
  {
    return false;
  }
  for (std::string::size_type i = lastPercentage + 2; i < suffix.size() - 1; ++i)
  {
    if (!isdigit(suffix[i]))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkFileSeriesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
