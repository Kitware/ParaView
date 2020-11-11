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

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkFileSeriesReader.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTypeTraits.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <algorithm>
#include <ctype.h> // for isprint().
#include <map>
#include <set>
#include <string>
#include <vector>

#include "vtk_jsoncpp.h"

//=============================================================================
vtkStandardNewMacro(vtkFileSeriesReader);
vtkInformationKeyMacro(vtkFileSeriesReader, FILE_SERIES_NUMBER_OF_FILES, Integer);
vtkInformationKeyMacro(vtkFileSeriesReader, FILE_SERIES_CURRENT_FILE_NUMBER, Integer);
vtkInformationKeyMacro(vtkFileSeriesReader, FILE_SERIES_FIRST_FILENAME, String);
//=============================================================================
// Internal class for holding time ranges.
class vtkFileSeriesReaderTimeRanges
{
public:
  void Reset();
  void AddTimeRange(int index, vtkInformation* srcInfo);
  int GetAggregateTimeInfo(vtkInformation* outInfo);
  int GetInputTimeInfo(int index, vtkInformation* outInfo);
  int GetIndexForTime(double time);
  int ChooseInput(vtkInformation* outInfo);
  std::vector<double> GetTimesForInput(int inputId, vtkInformation* outInfo);

  // When set to true, `GetAggregateTimeInfo` will not add any TIME_STEPS or
  // TIME_RANGE keys in the output information. This is useful to avoid adding
  // time for single-file reads where the internal reader did not report any
  // time nor did we have any overridden time.
  // See paraview/paraview#20314 and paraview/paraview#19777 for two apparently
  // conflicting use-cases that need to be supported.
  void SetSkipTimeInOutput(bool val) { this->SkipTimeInOutput = val; }
private:
  static vtkInformationIntegerKey* INDEX();
  typedef std::map<double, vtkSmartPointer<vtkInformation> > RangeMapType;
  RangeMapType RangeMap;
  std::map<int, vtkSmartPointer<vtkInformation> > InputLookup;
  bool SkipTimeInOutput = false;
};

vtkInformationKeyMacro(vtkFileSeriesReaderTimeRanges, INDEX, Integer);

//-----------------------------------------------------------------------------
void vtkFileSeriesReaderTimeRanges::Reset()
{
  this->RangeMap.clear();
  this->InputLookup.clear();
  this->SkipTimeInOutput = false;
}

//-----------------------------------------------------------------------------
void vtkFileSeriesReaderTimeRanges::AddTimeRange(int index, vtkInformation* srcInfo)
{
  VTK_CREATE(vtkInformation, info);
  info->Set(vtkFileSeriesReaderTimeRanges::INDEX(), index);

  this->InputLookup[index] = info;

  if (srcInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    info->CopyEntry(srcInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (srcInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      info->CopyEntry(srcInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
    else
    {
      double* timeSteps = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      int numTimeSteps = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      double timeRange[2];
      timeRange[0] = timeSteps[0];
      timeRange[1] = timeSteps[numTimeSteps - 1];
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  }
  else if (srcInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    info->CopyEntry(srcInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  else
  {
    vtkGenericWarningMacro(<< "Input with index " << index << " has no time information.");
    return;
  }

  this->RangeMap[info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE())[0]] = info;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReaderTimeRanges::GetAggregateTimeInfo(vtkInformation* outInfo)
{
  if (this->RangeMap.empty())
  {
    vtkGenericWarningMacro(<< "No inputs with time information.");
    return 0;
  }

  double timeRange[2];
  timeRange[0] =
    this->RangeMap.begin()->second->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE())[0];
  timeRange[1] =
    (--this->RangeMap.end())->second->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE())[1];

  // ensure time-range is valid.
  if (timeRange[0] > timeRange[1])
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    return 1;
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  std::vector<double> timeSteps;

  RangeMapType::iterator itr = this->RangeMap.begin();
  while (itr != this->RangeMap.end())
  {
    // First, get all the time steps for this input.
    double* localTimeSteps = itr->second->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numLocalSteps = itr->second->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double localEndTime = vtkTypeTraits<double>::Max();
    // Second, check where the time range for the next section begins.
    itr++;
    if (itr != this->RangeMap.end())
    {
      localEndTime = itr->second->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE())[0];
    }
    // Third, copy the appropriate time steps to the aggregate time steps.
    for (int i = 0; (i < numLocalSteps) && (localTimeSteps[i] < localEndTime); i++)
    {
      timeSteps.push_back(localTimeSteps[i]);
    }
  }

  if (timeSteps.size() > 0)
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0],
      static_cast<int>(timeSteps.size()));
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  // special-case: this is used to avoid making up time for non-temporal
  // datasets as that can cause unnecessary updates (see
  // paraview/paraview#20314).
  if (this->SkipTimeInOutput)
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReaderTimeRanges::GetInputTimeInfo(int index, vtkInformation* outInfo)
{
  if (this->InputLookup.find(index) == this->InputLookup.end())
  {
    // if there are no files specified, there's no time information to provide.
    return 1;
  }

  vtkInformation* storedInfo = this->InputLookup[index];
  outInfo->CopyEntry(storedInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  if (storedInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->CopyEntry(storedInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    return 1;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReaderTimeRanges::GetIndexForTime(double time)
{
  if (this->RangeMap.empty())
  {
    // It would make sense to give a warning here, but we should have already
    // warned in GetAggregateTimeInfo.  Warning here would just be annoying.
    return 0;
  }

  // This returns the item _after_ the one we want.
  RangeMapType::iterator itr = this->RangeMap.upper_bound(time);
  if (itr == this->RangeMap.begin())
  {
    // The requested time step is before any available time.  We will use it by
    // doing nothing here.
  }
  else
  {
    // Back up one to the item we really want.
    itr--;
  }

  return itr->second->Get(vtkFileSeriesReaderTimeRanges::INDEX());
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReaderTimeRanges::ChooseInput(vtkInformation* outInfo)
{
  int index = -1;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    index = this->GetIndexForTime(upTime);
  }
  else
  {
    index = 0;
  }

  return index;
}

//-----------------------------------------------------------------------------
std::vector<double> vtkFileSeriesReaderTimeRanges::GetTimesForInput(
  int inputId, vtkInformation* outInfo)
{
  // Get the saved info for this input.
  vtkInformation* inInfo = this->InputLookup[inputId];

  // This is the time range that is supported by this input.
  double* supportedTimeRange = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  // Get the time range from which we "allow" data from this input.  The lower
  // bound is simply the bottom part of the time range of the input, unless it
  // has the smallest time values, in which case it also defines all times less
  // than that.  The upper bound is defined where the input with the next
  // highest times starts.
  double allowedTimeRange[2];
  allowedTimeRange[0] = supportedTimeRange[0];

  // Find the input with the next times.
  RangeMapType::iterator itr = this->RangeMap.upper_bound(allowedTimeRange[0]);
  if (itr != this->RangeMap.end())
  {
    allowedTimeRange[1] = itr->first;
  }
  else
  {
    // There is no next time.
    allowedTimeRange[1] = vtkTypeTraits<double>::Max();
  }

  // Adjust the beginning time if we are the first time.
  if (this->RangeMap.find(allowedTimeRange[0]) == this->RangeMap.begin())
  {
    allowedTimeRange[0] = -vtkTypeTraits<double>::Max();
  }

  // Now we are finally ready to identify the times
  std::vector<double> times;

  // Get the update times
  int numUpTimes = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  double upTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  for (int i = 0; i < numUpTimes; i++)
  {
    if ((upTime >= allowedTimeRange[0]) && (upTime < allowedTimeRange[1]))
    {
      // Add the time.  Clamp it to the input's supported time range in
      // case that input is clipping based on the time.
      times.push_back(std::max(supportedTimeRange[0], std::min(supportedTimeRange[1], upTime)));
    }
  }

  return times;
}

namespace
{
// Helper class used to ensure that ProcessRequest() never results in change
// in MTime as that can have disastrous effects.
class vtkEnsureMTime
{
  vtkObject* Object;
  vtkMTimeType MTime;

public:
  vtkEnsureMTime(vtkObject* object)
    : Object(object)
    , MTime(object ? object->GetMTime() : 0)
  {
  }
  ~vtkEnsureMTime()
  {
    if (this->Object && this->Object->GetMTime() != this->MTime)
    {
      cerr << this->Object->GetClassName()
           << "'s MTime was changed unexpectedly.\n"
              "This can imply serious problem in the reader logic and cause\n"
              "unexpected issues when running in parallel. \n"
              "Please address the issues."
           << endl;
      abort();
    }
  }
};

class vtkRecordMTime
{
  vtkObject* Object;
  vtkMTimeType& MTime;

public:
  vtkRecordMTime(vtkObject* obj, vtkMTimeType& mtime)
    : Object(obj)
    , MTime(mtime)
  {
    this->MTime = 0;
  }
  ~vtkRecordMTime()
  {
    if (this->Object)
    {
      this->MTime = this->Object->GetMTime();
    }
  }

private:
  void operator=(const vtkRecordMTime&);
};
}

//=============================================================================
struct vtkFileSeriesReaderInternals
{
  std::vector<std::string> FileNames;
  std::vector<std::string> RealFileNames;
  std::vector<double> TimeValues;
  bool FileNameIsSet;
  vtkFileSeriesReaderTimeRanges* TimeRanges;
};

//=============================================================================
vtkFileSeriesReader::vtkFileSeriesReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internal = new vtkFileSeriesReaderInternals;
  this->Internal->FileNameIsSet = false;
  this->Internal->TimeRanges = new vtkFileSeriesReaderTimeRanges;

  this->UseMetaFile = 0;
  this->UseJsonMetaFile = false;

  this->IgnoreReaderTime = false;
}

//-----------------------------------------------------------------------------
vtkFileSeriesReader::~vtkFileSeriesReader()
{
  delete this->Internal->TimeRanges;
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::AddFileName(const char* name)
{
  this->AddFileNameInternal(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::RemoveAllFileNames()
{
  this->RemoveAllFileNamesInternal();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::RemoveAllFileNamesInternal()
{
  this->Internal->FileNames.clear();
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::AddFileNameInternal(const char* name)
{
  this->Internal->FileNames.push_back(name);
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::RemoveAllRealFileNamesInternal()
{
  this->Internal->RealFileNames.clear();
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::CopyRealFileNamesFromFileNames()
{
  this->Internal->RealFileNames = this->Internal->FileNames;
}

//----------------------------------------------------------------------------
unsigned int vtkFileSeriesReader::GetNumberOfFileNames()
{
  return static_cast<unsigned int>(this->Internal->RealFileNames.size());
}

//----------------------------------------------------------------------------
const char* vtkFileSeriesReader::GetFileName(unsigned int idx)
{
  if (idx >= this->Internal->RealFileNames.size())
  {
    return 0;
  }
  return this->Internal->RealFileNames[idx].c_str();
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::CanReadFile(const char* filename)
{
  if (!this->Reader)
  {
    return 0;
  }

  if (!this->UseMetaFile && !this->UseJsonMetaFile)
  {
    if (filename)
    {
      std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
      if (ext == ".series")
      {
        this->UseJsonMetaFile = true;
      }
    }
  }

  if (this->UseMetaFile || this->UseJsonMetaFile)
  {
    // filename really points to a metafile.
    VTK_CREATE(vtkStringArray, dataFiles);
    std::vector<double> timeValues;
    if (this->ReadMetaDataFile(filename, dataFiles, timeValues, 1))
    {
      if (dataFiles->GetNumberOfValues() > 0)
      {
        return ReaderCanReadFile(dataFiles->GetValue(0).c_str());
      }
    }
    return 0;
  }
  else
  {
    return ReaderCanReadFile(filename);
  }
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkEnsureMTime check(this);

  this->UpdateMetaData();

  if (this->Reader)
  {
    // We want to suppress the modification time change in the Reader.  See
    // vtkFileSeriesReader::GetMTime() for details on how this works.
    this->BeforeFileNameMTime = this->GetMTime();
    vtkRecordMTime record_time(this->Reader, this->FileNameMTime);

    // Make sure that there is a file to get information from.
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
      if (!this->Internal->FileNameIsSet && (this->GetNumberOfFileNames() > 0))
      {
        this->ReaderSetFileName(this->GetFileName(0));
        this->_FileIndex = 0;
        this->Internal->FileNameIsSet = true;
      }
    }
    // Our handling of these requests will call the reader's request in turn.
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      return this->RequestInformation(request, inputVector, outputVector);
    }
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
      return this->RequestData(request, inputVector, outputVector);
    }

    // Additional processing required by us.
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      this->RequestUpdateExtent(request, inputVector, outputVector);
    }

    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
    {
      this->RequestUpdateTime(request, inputVector, outputVector);
    }

    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_TIME_DEPENDENT_INFORMATION()))
    {
      this->RequestUpdateTimeDependentInformation(request, inputVector, outputVector);
    }

    // Expose number of files and first filename as
    // information keys for potential use in the internal reader
    outputVector->GetInformationObject(0)->Set(
      FILE_SERIES_NUMBER_OF_FILES(), this->GetNumberOfFileNames());
    outputVector->GetInformationObject(0)->Set(FILE_SERIES_FIRST_FILENAME(), this->GetFileName(0));

    // Let the reader process anything we did not handle ourselves.
    int retVal = this->Reader->ProcessRequest(request, inputVector, outputVector);

    return retVal;
  }
  vtkErrorMacro("No reader is defined. Cannot perform pipeline pass.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkFileSeriesReader::ResetTimeRanges()
{
  this->Internal->TimeRanges->Reset();
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestInformation(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  this->ResetTimeRanges();

  int requestFromPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  assert(requestFromPort < this->GetNumberOfOutputPorts());

  vtkInformation* outInfo = outputVector->GetInformationObject(requestFromPort);
  unsigned int numFiles = this->GetNumberOfFileNames();
  if (numFiles < 1)
  {
    // This can happen in special cases, like Plot3DReader where the
    // vtkFileSeriesReader is actually controlling a non-essential file-name
    // property. In which case, we simply pass the RequestInformation call to
    // the reader.
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

    // Expose current file number as information key for potential use in the internal reader
    outputVector->GetInformationObject(requestFromPort)->Set(FILE_SERIES_CURRENT_FILE_NUMBER(), 0);
    this->RequestInformationForInput(0, request, outputVector);

    // vtkErrorMacro("Expecting at least 1 file.  Cannot proceed.");
    return 1;
  }

  // Run RequestInformation on the reader for the first file.  Use that info to
  // determine if the inputs have time information
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  // Expose current file number as information key for potential use in the internal reader
  outputVector->GetInformationObject(requestFromPort)->Set(FILE_SERIES_CURRENT_FILE_NUMBER(), 0);
  this->RequestInformationForInput(0, request, outputVector);

  bool ignoreReaderTime = this->IgnoreReaderTime;
  ignoreReaderTime |= !this->Internal->TimeValues.empty();

  // Does the reader have time?
  if (ignoreReaderTime || (!outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) &&
                            !outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE())))
  {
    // Input files have no time steps.  Fake a time step for each equal to the
    // index.
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    const bool override_time =
      numFiles > 1 || (this->UseJsonMetaFile && this->Internal->TimeValues.size() == numFiles);
    this->Internal->TimeRanges->SetSkipTimeInOutput(override_time == false);
    for (unsigned int i = 0; i < numFiles; i++)
    {
      double time = (double)i;
      if (this->UseJsonMetaFile && this->Internal->TimeValues.size() == numFiles)
      {
        time = this->Internal->TimeValues[i];
      }
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time, 1);
      this->Internal->TimeRanges->AddTimeRange(static_cast<int>(i), outInfo);
    }
  }
  else
  {
    // Record the reported file time info.
    this->Internal->TimeRanges->AddTimeRange(0, outInfo);

    // Query all the other files for time info.
    for (unsigned int i = 1; i < numFiles; i++)
    {
      // Expose current file number as information key for potential use in the internal reader
      outputVector->GetInformationObject(requestFromPort)
        ->Set(FILE_SERIES_CURRENT_FILE_NUMBER(), static_cast<int>(i));
      this->RequestInformationForInput(static_cast<int>(i), request, outputVector);
      this->Internal->TimeRanges->AddTimeRange(static_cast<int>(i), outInfo);
    }
  }

  // Now that we have collected all of the time information, set the aggregate
  // time steps in the output.
  this->Internal->TimeRanges->GetAggregateTimeInfo(outInfo);

  vtkLogF(TRACE, "%s: has time: %d", vtkLogIdentifier(this),
    outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  int requestFromPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  assert(requestFromPort < this->GetNumberOfOutputPorts());

  vtkInformation* outInfo = outputVector->GetInformationObject(requestFromPort);
  int index = this->ChooseInput(outInfo);
  if (index >= static_cast<int>(this->GetNumberOfFileNames()))
  {
    // this happens when there are no files set. That's an acceptable condition
    // when the file-series is not an essential filename eg. the Q file for
    // Plot3D reader.
    index = -1;
  }
  if (index < 0)
  {
    vtkDebugMacro("Inputs are not set.");
    return 0;
  }

  // Make sure that the reader file name is set correctly and that
  // RequestInformation has been called.
  outputVector->GetInformationObject(requestFromPort)
    ->Set(FILE_SERIES_CURRENT_FILE_NUMBER(), index);
  this->RequestInformationForInput(index);

// I commented out the following block because it is probably not important
// and it is causing a crash in some circumstances (bug #7253).
#if 0
  // I'm not sure if the executive will wipe out this information before
  // RequestData is called, and I don't think I care.  I'm just setting this
  // here for completeness.
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    std::vector<double> times
      = this->Internal->TimeRanges->GetTimesForInput(index, outInfo);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                 &times[0], times.size());
    }
#endif
  return 1;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int requestFromPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  assert(requestFromPort < this->GetNumberOfOutputPorts());

  // We have modified the TIME_STEPS information in the output vector.  Some
  // readers (e.g. the Exodus reader) reuse this array to get time indices.
  // Just in case, restore the vector.
  vtkInformation* outInfo = outputVector->GetInformationObject(requestFromPort);
  this->Internal->TimeRanges->GetInputTimeInfo(this->_FileIndex, outInfo);

  int retVal = this->Reader->ProcessRequest(request, inputVector, outputVector);

  if (this->GetNumberOfFileNames() > 0)
  {
    // Now restore the information.
    this->Internal->TimeRanges->GetAggregateTimeInfo(outInfo);
  }

  return retVal;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReader::RequestInformationForInput(
  int index, vtkInformation* request, vtkInformationVector* outputVector)
{
  if ((index != this->_FileIndex) || (outputVector != NULL))
  {
    if (this->GetNumberOfFileNames() > 0)
    {
      this->ReaderSetFileName(this->GetFileName(index));
    }
    else
    {
      this->ReaderSetFileName(0);
    }

    this->_FileIndex = index;
    // Need to call RequestInformation on reader to refresh any metadata for the
    // new filename.
    vtkSmartPointer<vtkInformation> tempRequest;
    if (request)
    {
      tempRequest = request;
    }
    else
    {
      tempRequest = vtkSmartPointer<vtkInformation>::New();
      tempRequest->Set(vtkDemandDrivenPipeline::REQUEST_INFORMATION());
    }
    vtkSmartPointer<vtkInformationVector> tempOutputVector;
    if (outputVector)
    {
      tempOutputVector = outputVector;
    }
    else
    {
      tempOutputVector = vtkSmartPointer<vtkInformationVector>::New();
      for (int cc = 0; cc < this->GetNumberOfOutputPorts(); ++cc)
      {
        VTK_CREATE(vtkInformation, tempOutputInfo);
        tempOutputVector->Append(tempOutputInfo);
      }
    }
    return this->Reader->ProcessRequest(
      tempRequest, (vtkInformationVector**)NULL, tempOutputVector);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReader::FillOutputPortInformation(int port, vtkInformation* info)
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
int vtkFileSeriesReader::ReadMetaDataFile(const char* metafilename, vtkStringArray* filesToRead,
  std::vector<double>& timeValues, int maxFilesToRead /*= VTK_INT_MAX*/)
{
  // Open the metafile.
  vtksys::ifstream metafile(metafilename);
  if (metafile.bad())
  {
    return 0;
  }

  if (this->UseJsonMetaFile)
  {
    Json::Value root;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    bool parsingSuccessful = parseFromStream(builder, metafile, &root, nullptr);
    if (parsingSuccessful)
    {
      if (!root.isMember("file-series-version"))
      {
        vtkErrorMacro("Syntax error in meta-file. A list of file names is required.");
        return 0;
      }
      if (!root.isMember("files"))
      {
        vtkErrorMacro("Syntax error in meta-file. A list of file names is required.");
        return 0;
      }
      const Json::Value& filenames = root["files"];
      for (size_t index = 0; index < filenames.size(); ++index)
      {
        const Json::Value& astep = filenames[(int)index];
        if (astep.isString())
        {
          std::string name = astep.asString();
          filesToRead->InsertNextValue(FromRelativeToMetaFile(metafilename, name.c_str()));
        }
        else if (astep.isObject())
        {
          if (!astep.isMember("name"))
          {
            vtkErrorMacro("Syntax error in meta-file. A filename is required.");
            filesToRead->Initialize();
            timeValues.clear();
            return 0;
          }
          std::string name = astep["name"].asString();
          filesToRead->InsertNextValue(FromRelativeToMetaFile(metafilename, name.c_str()));
          if (!astep.isMember("time"))
          {
            vtkErrorMacro("Syntax error in meta-file. A time value is required.");
            filesToRead->Initialize();
            timeValues.clear();
            return 0;
          }
          double value = astep["time"].asDouble();
          timeValues.push_back(value);
        }
        else
        {
          vtkErrorMacro("Syntax error in meta-file. Can't read.");
          return 0;
        }
      }
      return 1;
    }
    else
    {
      vtkErrorMacro("Syntax error in meta-file. Can't read.");
      return 0;
    }
    return 1;
  }
  else
  {
    // Iterate over all files pointed to by the metafile.
    filesToRead->SetNumberOfTuples(0);
    filesToRead->SetNumberOfComponents(1);
    while (
      metafile.good() && !metafile.eof() && (filesToRead->GetNumberOfTuples() < maxFilesToRead))
    {
      std::string fname;
      metafile >> fname;
      if (fname.empty())
        continue;
      for (size_t cc = 0; cc < fname.size(); cc++)
      {
        int ch = fname[cc];
        // to avoid assert() in debug MSVC, we need to ensure 0 <= ch < 256.
        if (static_cast<unsigned int>(ch) >= 256 || !isprint(ch))
        {
          // must not be an ASCII file.
          return 0;
        }
      }
      filesToRead->InsertNextValue(FromRelativeToMetaFile(metafilename, fname.c_str()));
    }
    return 1;
  }
}

//-----------------------------------------------------------------------------
void vtkFileSeriesReader::UpdateMetaData()
{
  if (this->MetaFileReadTime > this->GetMTime())
  {
    return;
  }

  if (!this->UseMetaFile)
  {
    if (!this->Internal->FileNames.empty())
    {
      const char* fname = this->Internal->FileNames[0].c_str();
      if (fname)
      {
        std::string ext = vtksys::SystemTools::GetFilenameLastExtension(fname);
        if (ext == ".series")
        {
          this->UseJsonMetaFile = true;
          this->MetaFileNameMTime = this->vtkDataObjectAlgorithm::GetMTime();
          size_t len = strlen(fname) + 1;
          if (this->_MetaFileName)
          {
            delete[] this->_MetaFileName;
          }
          this->_MetaFileName = new char[len];
          memcpy(this->_MetaFileName, fname, len);
        }
      }
    }
  }

  if (this->UseMetaFile || this->UseJsonMetaFile)
  {
    VTK_CREATE(vtkStringArray, dataFiles);
    std::vector<double> timeValues;
    if (!this->ReadMetaDataFile(this->_MetaFileName, dataFiles, timeValues))
    {
      vtkErrorMacro(<< "Could not open metafile " << this->_MetaFileName);
      return;
    }

    vtkIdType numFiles = dataFiles->GetNumberOfValues();
    // essential that we don't use the public methods AddFileName(),
    // RemoveAllFileNames() since those change the MTime of this class in
    // ProcessRequest() method.
    this->Internal->RealFileNames.clear();
    for (int i = 0; i < numFiles; i++)
    {
      this->Internal->RealFileNames.push_back(dataFiles->GetValue(i));
    }

    this->Internal->TimeValues.clear();
    if ((size_t)numFiles == timeValues.size())
    {
      for (int i = 0; i < numFiles; i++)
      {
        this->Internal->TimeValues.push_back(timeValues[i]);
      }
    }
  }
  else
  {
    this->CopyRealFileNamesFromFileNames();
  }

  this->MetaFileReadTime.Modified();
}

//-----------------------------------------------------------------------------
const char* vtkFileSeriesReader::GetCurrentFileName()
{
  return this->GetFileName(this->_FileIndex);
}

//-----------------------------------------------------------------------------
void vtkFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MetaFileName: " << (this->_MetaFileName ? this->_MetaFileName : "(none)")
     << endl;
  os << indent << "UseMetaFile: " << this->UseMetaFile << endl;
  os << indent << "IgnoreReaderTime: " << this->IgnoreReaderTime << endl;
}

//-----------------------------------------------------------------------------
int vtkFileSeriesReader::ChooseInput(vtkInformation* outInfo)
{
  return this->Internal->TimeRanges->ChooseInput(outInfo);
}
