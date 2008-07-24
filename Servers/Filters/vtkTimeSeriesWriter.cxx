/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeSeriesWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimeSeriesWriter.h"

#include "vtkAppendPolyData.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

#include <vtkstd/string>

vtkStandardNewMacro(vtkTimeSeriesWriter);
vtkCxxRevisionMacro(vtkTimeSeriesWriter, "1.1");

vtkCxxSetObjectMacro(vtkTimeSeriesWriter,Writer,vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkTimeSeriesWriter::vtkTimeSeriesWriter()
{
  this->SetNumberOfOutputPorts(0);

  this->Writer = 0;

  this->FileNameMethod = 0;
  this->FileName = 0;

  this->WriteAllTimeSteps = 0;
  
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;
}

//-----------------------------------------------------------------------------
vtkTimeSeriesWriter::~vtkTimeSeriesWriter()
{
  if (this->Writer)
    {
    this->Writer->Delete();
    }
  this->SetFileNameMethod(0);
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
int vtkTimeSeriesWriter::Write()
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
int vtkTimeSeriesWriter::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps = 
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeSeriesWriter::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Writer->ProcessRequest(request, inputVector, outputVector))
    {
    return 0;
    }
  
  // get the requested update extent
  double *inTimes = inputVector[0]->GetInformationObject(0)->Get(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
    {
    double timeReq = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set( 
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
        &timeReq, 1);
    }
  
  return 1;  
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkTimeSeriesWriter::AppendBlocks(vtkCompositeDataSet* cds)
{
  // For now, we handle composite of poly data. This will change to
  // only composite of vtkTable in the future.
  vtkSmartPointer<vtkAppendPolyData> append =
    vtkSmartPointer<vtkAppendPolyData>::New();
  
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(cds->NewIterator());
  int numInputs = 0;
  for(iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataObject* curObj = iter->GetCurrentDataObject();
    if (curObj->IsA("vtkPolyData"))
      {
      numInputs++;
      append->AddInput(static_cast<vtkPolyData*>(curObj));
      }
    }
    
  vtkRectilinearGrid* output = vtkRectilinearGrid::New();
  if (numInputs > 0)
    {
    append->Update();
    vtkPolyData* appended = append->GetOutput(); 
    output->SetDimensions(appended->GetNumberOfPoints(), 1, 1);
    vtkFloatArray* tmp = vtkFloatArray::New();
    tmp->SetNumberOfTuples(appended->GetNumberOfPoints());
    tmp->FillComponent(0, 0);
    output->SetXCoordinates(tmp);
    tmp->Delete();
    tmp = vtkFloatArray::New();
    tmp->SetNumberOfTuples(1);
    tmp->SetValue(0, 0);
    output->SetYCoordinates(tmp);
    output->SetZCoordinates(tmp);
    tmp->Delete();
    output->GetPointData()->ShallowCopy(appended->GetPointData());
    vtkDataArray* pts = appended->GetPoints()->GetData();
    if (pts->GetName())
      {
      output->GetPointData()->AddArray(pts);
      }
    else
      {
      vtkDataArray* newpts = pts->NewInstance();
      newpts->DeepCopy(pts);
      newpts->SetName("Positions");
      output->GetPointData()->AddArray(newpts);
      newpts->Delete();
      }
    }
  return output;
}

//----------------------------------------------------------------------------
int vtkTimeSeriesWriter::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Writer)
    {
    vtkErrorMacro("No internal writer specified. Cannot write.");
    return 0;
    }

  if (this->WriteAllTimeSteps)
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
  
  vtkSmartPointer<vtkDataObject> inputCopy;
  if (input->IsA("vtkCompositeDataSet"))
    {
    inputCopy.TakeReference(this->AppendBlocks(
        static_cast<vtkCompositeDataSet*>(input)));
    }
  else
    {
    inputCopy.TakeReference(input->NewInstance());
    inputCopy->ShallowCopy(input);
    }

  vtksys_ios::ostringstream fname;
  if (this->WriteAllTimeSteps)
    {
    vtkstd::string path = 
      vtksys::SystemTools::GetFilenamePath(this->FileName);
    vtkstd::string fnamenoext =
      vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    vtkstd::string ext =
      vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    fname << path << "/" << fnamenoext << this->CurrentTimeIndex << ext;
    }
  else
    {
    fname << this->FileName;
    }
  
  this->WriteAFile(fname.str().c_str(), inputCopy);

  if (this->WriteAllTimeSteps)
    {
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
      {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkTimeSeriesWriter::WriteAFile(const char* fname,
    vtkDataObject* input)
{
  this->Writer->SetInputConnection(input->GetProducerPort());
  this->SetWriterFileName(fname);

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
  
  this->Writer->SetInputConnection(0);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is 
// modified, then this object is modified as well.
unsigned long vtkTimeSeriesWriter::GetMTime()
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
void vtkTimeSeriesWriter::SetWriterFileName(const char* fname)
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
void vtkTimeSeriesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
