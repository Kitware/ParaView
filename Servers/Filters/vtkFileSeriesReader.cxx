/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFileSeriesReader.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

#include <vtkstd/vector>
#include <vtkstd/string>

vtkStandardNewMacro(vtkFileSeriesReader);
vtkCxxRevisionMacro(vtkFileSeriesReader, "1.5");

vtkCxxSetObjectMacro(vtkFileSeriesReader,Reader,vtkAlgorithm);

struct vtkFileSeriesReaderInternals
{
  vtkstd::vector<vtkstd::string> FileNames;
  bool FileNameIsSet;
};

//-----------------------------------------------------------------------------
vtkFileSeriesReader::vtkFileSeriesReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Reader = 0;

  this->Internal = new vtkFileSeriesReaderInternals;
  this->Internal->FileNameIsSet = false;

  this->FileNameMethod = 0;
  //this->SetFileNameMethod("SetFileName");
  this->SetFileNameMethod(0);
}

//-----------------------------------------------------------------------------
vtkFileSeriesReader::~vtkFileSeriesReader()
{
  if (this->Reader)
    {
    this->Reader->Delete();
    }
  delete this->Internal;
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is 
// modified, then this object is modified as well.
unsigned long vtkFileSeriesReader::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long readerMTime;

  if ( this->Reader )
    {
    readerMTime = this->Reader->GetMTime();
    mTime = ( readerMTime > mTime ? readerMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::AddFileName(const char* name)
{
  this->Internal->FileNames.push_back(name);
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::RemoveAllFileNames()
{
  this->Internal->FileNames.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkFileSeriesReader::GetNumberOfFileNames()
{
  return this->Internal->FileNames.size();
}

//----------------------------------------------------------------------------
const char* vtkFileSeriesReader::GetFileName(unsigned int idx)
{
  if (idx >= this->Internal->FileNames.size())
    {
    return 0;
    }
  return this->Internal->FileNames[idx].c_str();
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::CanReadFile(const char* name)
{
  if (!this->Reader)
    {
    return 0;
    }
  vtkClientServerID csId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(this->Reader);
  if (csId.ID)
    {
    int canRead = 1;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    // Get the local process interpreter.
    vtkClientServerInterpreter* interp = pm->GetInterpreter();
    vtkClientServerStream stream;
    // Pass the CanReadFile to the internal reader. Turn off
    // ReportInterpreterErrors in case the internal reader does
    // not have a CanReadFile
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << csId << "CanReadFile" << name
           << vtkClientServerStream::End;
    interp->ProcessStream(stream);
    interp->GetLastResult().GetArgument(0, 0, &canRead);
    stream.Reset();
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 1
           << vtkClientServerStream::End;
    interp->ProcessStream(stream);
    return canRead;
    }
  return 0;

}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::ProcessRequest(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  if (this->Reader)
    {
    // Make sure that there is a file to get information from.
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
      {
      if (!this->Internal->FileNameIsSet && !this->Internal->FileNames.empty())
        {
        this->SetReaderFileName(this->Internal->FileNames[0].c_str());
        this->Internal->FileNameIsSet = true;
        }
      }
    int retVal = this->Reader->ProcessRequest(request, 
                                              inputVector,
                                              outputVector);
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
      {
      this->RequestInformation(request, inputVector, outputVector);
      }
    else if (
      request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
      {
      this->RequestUpdateExtent(request, inputVector, outputVector);
      }

    return retVal;
    }
  vtkErrorMacro("No reader is defined. Cannot perform pipeline pass.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  unsigned int numTimeSteps = this->Internal->FileNames.size();
  if (numTimeSteps > 1)
    {
    double* timesteps = new double[numTimeSteps];
    for(unsigned int i=0; i<numTimeSteps; i++)
      {
      timesteps[i] = i;
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                 timesteps, 
                 numTimeSteps);
    double timeRange[2];
    timeRange[0] = 0;
    timeRange[1] = numTimeSteps - 1;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
                 timeRange, 2);
    }
  else if (numTimeSteps == 0)
    {
    vtkErrorMacro("Expecting at least 1 file. Cannot proceed.");
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  unsigned int numTimeSteps = this->Internal->FileNames.size();
  const char* fname = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    if(numTimeSteps > 1)
      {
      // Get the requested time step. We only supprt requests of a single time
      // step in this reader right now
      double *requestedTimeSteps = 
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
      unsigned int updateTime = vtkMath::Round(requestedTimeSteps[0]);
      
      if (updateTime >= numTimeSteps)
        {
        updateTime = numTimeSteps - 1;
        }
      
      fname = this->GetFileName(updateTime);
      }
    else if (numTimeSteps == 1)
      {
      fname = this->GetFileName(0);
      }
    }
  
  this->SetReaderFileName(fname);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkFileSeriesReader::SetReaderFileName(const char* fname)
{
  if (this->Reader && fname)
    {
    vtkClientServerID csId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(this->Reader);
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
int vtkFileSeriesReader::FillOutputPortInformation(int port, 
                                                   vtkInformation* info)
{
  if (this->Reader)
    {
    vtkInformation* rinfo = this->Reader->GetOutputPortInformation(port);
    info->CopyEntry(rinfo, vtkDataObject::DATA_TYPE_NAME());
    return 1;
    }
  vtkErrorMacro("No reader is defined. Cannot provide output information.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
