/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaDatabaseReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCinemaDatabaseReader.h"

#include "vtkCinemaDatabase.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkCinemaDatabaseReader);
//----------------------------------------------------------------------------
vtkCinemaDatabaseReader::vtkCinemaDatabaseReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->PipelineObject = NULL;
}

//----------------------------------------------------------------------------
vtkCinemaDatabaseReader::~vtkCinemaDatabaseReader()
{
  this->SetFileName(NULL);
  this->SetPipelineObject(NULL);
}

//----------------------------------------------------------------------------
int vtkCinemaDatabaseReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->FileName || this->FileName[0] == 0)
  {
    vtkErrorMacro("'FileName' must be specified.");
    return 0;
  }

  if (!this->Helper->Load(this->FileName))
  {
    vtkErrorMacro("Failed to load database: " << this->FileName);
    return 0;
  }

  // Announce timesteps available, if any.
  // For Cinema, timesteps are simply strings, while in ParaView they are
  // doubles.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->TimeStepsMap.clear();

  std::vector<std::string> tstepStrings = this->Helper->GetTimeSteps();
  std::vector<double> tsteps;
  for (std::vector<std::string>::iterator iter = tstepStrings.begin(); iter != tstepStrings.end();
       ++iter)
  {
    double time = atof(iter->c_str());
    this->TimeStepsMap[time] = *iter;
    tsteps.push_back(time);
  }
  if (tsteps.size() > 0)
  {
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &tsteps[0], static_cast<int>(tsteps.size()));
    double timeRange[2];
    timeRange[0] = tsteps.front();
    timeRange[1] = tsteps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCinemaDatabaseReader::ClearControlParameter(const char* pname)
{
  this->EnabledControlParameterValues.erase(pname);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCinemaDatabaseReader::EnableControlParameterValue(const char* pname, int value_index)
{
  std::vector<std::string> values = this->Helper->GetControlParameterValues(pname);
  if (value_index >= 0 && value_index < static_cast<int>(values.size()))
  {
    this->EnableControlParameterValue(pname, values[value_index].c_str());
  }
}

//----------------------------------------------------------------------------
void vtkCinemaDatabaseReader::EnableControlParameterValue(const char* pname, const char* value)
{
  this->EnabledControlParameterValues[pname].insert(value);
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkCinemaDatabaseReader::GetQueryString(double time) const
{
  std::ostringstream str;
  // str << "{";
  str << "'vis': ['" << this->PipelineObject << "']";

  TimeStepsMapType::const_iterator timeIter = this->TimeStepsMap.find(time);
  if (timeIter != this->TimeStepsMap.end())
  {
    str << ", 'time' : [ '" << timeIter->second.c_str() << "']";
  }
  for (MapOfVectorOfString::const_iterator miter = this->EnabledControlParameterValues.begin();
       miter != this->EnabledControlParameterValues.end(); ++miter)
  {
    str << ", ";
    str << "'" << miter->first.c_str() << "' : [ ";
    for (SetOfStrings::const_iterator siter = miter->second.begin(); siter != miter->second.end();
         ++siter)
    {
      if (siter != miter->second.begin())
      {
        str << ", ";
      }
      str << siter->c_str();
    }
    str << "]";
  }
  // str << "}";
  // I am deliberately ditching the start/end markers.
  return str.str();
}

//----------------------------------------------------------------------------
int vtkCinemaDatabaseReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->PipelineObject || this->PipelineObject[0] == '\0')
  {
    vtkErrorMacro("'PipelineObject' name not set.");
    return 0;
  }

  if (this->EnabledControlParameterValues.size() == 0 &&
    this->Helper->GetControlParameters(this->PipelineObject).size() != 0)
  {
    // nothing enabled, nothing to show.
    return 1;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double time = 0.0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  else if (this->TimeStepsMap.size() > 0)
  {
    time = this->TimeStepsMap.begin()->first;
  }

  // clamp time
  if (this->TimeStepsMap.size() > 0)
  {
    if (time < this->TimeStepsMap.begin()->first)
    {
      time = this->TimeStepsMap.begin()->first;
    }
    else if (time > this->TimeStepsMap.rbegin()->first)
    {
      time = this->TimeStepsMap.rbegin()->first;
    }
    else
    {
      time = this->TimeStepsMap.lower_bound(time)->first;
    }
  }

  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(2);
  pts->SetPoint(0, 0, 0, 0);
  pts->SetPoint(1, 0, 0, 0);

  output->SetPoints(pts.Get());

  vtkDataSetAttributes* pd = output->GetPointData();

  std::vector<std::string> fields = this->Helper->GetFieldValues(this->PipelineObject, "value");
  // TODO: handle multi-component fields.
  for (std::vector<std::string>::iterator iter = fields.begin(); iter != fields.end(); ++iter)
  {
    double range[2];
    if (this->Helper->GetFieldValueRange(this->PipelineObject, *iter, range))
    {
      vtkNew<vtkDoubleArray> array;
      array->SetName(iter->c_str());
      array->SetNumberOfTuples(2);
      array->SetTypedComponent(0, 0, range[0]);
      array->SetTypedComponent(1, 0, range[1]);
      pd->AddArray(array.Get());
    }
  }

  if (fields.size() == 0)
  {
    fields = this->Helper->GetFieldValues(this->PipelineObject, "lut");
    // TODO: handle multi-component fields.
    for (std::vector<std::string>::iterator iter = fields.begin(); iter != fields.end(); ++iter)
    {
      double range[2];
      if (this->Helper->GetFieldValueRange(this->PipelineObject, *iter, range))
      {
        vtkNew<vtkDoubleArray> array;
        array->SetName(iter->c_str());
        array->SetNumberOfTuples(2);
        array->SetTypedComponent(0, 0, range[0]);
        array->SetTypedComponent(1, 0, range[1]);
        pd->AddArray(array.Get());
      }
    }
  }

  // Now add cinema database information. The representation will use this
  // information to extract layers for the current view.
  vtkNew<vtkStringArray> sa;
  sa->SetName("CinemaDatabaseMetaData");
  sa->SetNumberOfTuples(4);
  sa->SetValue(0, this->FileName);
  sa->SetValue(1, this->PipelineObject);
  sa->SetValue(2, this->GetQueryString(time));

  TimeStepsMapType::const_iterator iter = this->TimeStepsMap.find(time);
  sa->SetValue(3, iter != this->TimeStepsMap.end() ? iter->second : std::string());
  output->GetFieldData()->AddArray(sa.Get());
  return 1;
}

//----------------------------------------------------------------------------
void vtkCinemaDatabaseReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << endl;
}
