/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotHistoryReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpyPlotHistoryReader.h"
#include "vtkSpyPlotHistoryReaderPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace SpyPlotHistoryReaderPrivate;
vtkStandardNewMacro(vtkSpyPlotHistoryReader);

//-----------------------------------------------------------------------------
// meta information of the file
class vtkSpyPlotHistoryReader::MetaInfo
{
public:
  MetaInfo()
  {
    TimeSteps.reserve(1024);
    MetaIndexes["time"] = -1;
  }
  ~MetaInfo() {}

  // rough bidirectional map of header index for time
  std::map<std::string, int> MetaIndexes;
  std::map<int, std::string> MetaLookUp;

  // maps the column index to the row/point/tracer id that the row represents
  std::map<int, int> ColumnIndexToTracerId;

  // maps the names for each col in the header
  // presumption is that all points are continuous
  // and the properties are in the same order for each point
  std::vector<std::string> Header;

  // maps col index to field property names
  std::map<int, std::string> FieldIndexesToNames;

  // lookup table of time info to file position
  std::vector<TimeStep> TimeSteps;
};

class vtkSpyPlotHistoryReader::CachedTables
{
public:
  std::vector<vtkTable*> Tables;
};
//-----------------------------------------------------------------------------
vtkSpyPlotHistoryReader::vtkSpyPlotHistoryReader()
  : Info(new MetaInfo)
{
  this->CachedOutput = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = 0;
  this->CommentCharacter = 0;
  this->Delimeter = 0;
  this->SetCommentCharacter("%");
  this->SetDelimeter(",");
}

//-----------------------------------------------------------------------------
vtkSpyPlotHistoryReader::~vtkSpyPlotHistoryReader()
{
  this->SetFileName(0);
  this->SetCommentCharacter(0);
  this->SetDelimeter(0);

  delete this->Info;
  if (this->CachedOutput)
  {
    size_t size = this->CachedOutput->Tables.size();
    for (size_t i = 0; i < size; ++i)
    {
      this->CachedOutput->Tables[i]->Delete();
    }
    delete this->CachedOutput;
  }
}

//----------------------------------------------------------------------------
void vtkSpyPlotHistoryReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Read the file to gather the number of timesteps
int vtkSpyPlotHistoryReader::RequestInformation(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  std::map<std::string, std::string> timeInfo;

  // we need to know the file position of the line before it is read in
  // not after
  std::streampos tellgValue;

  // Open the file and get the number time steps
  std::string line;
  std::ifstream file_stream(this->FileName, std::ifstream::in);
  int row = 0;
  while (file_stream.good())
  {
    tellgValue = file_stream.tellg();
    getline(file_stream, line);

    // skip any line that starts with a comment
    if (line.size() <= 1)
    {
      continue;
    }
    else if (line[0] == this->CommentCharacter[0])
    {
      continue;
    }

    if (row == 0)
    {
      // read the header
      // now find the cycle and time information and store those indexes
      getMetaHeaderInfo(line, this->Delimeter[0], this->Info->MetaIndexes, this->Info->MetaLookUp);

      // now convert this line into a table
      // we are going to have to reduce the header collection
      this->Info->Header = createTableLayoutFromHeader(line, this->Delimeter[0],
        this->Info->ColumnIndexToTracerId, this->Info->FieldIndexesToNames);

      // skip the next line if it contains the property types
      std::streampos peakLineG = file_stream.tellg();
      getline(file_stream, line);
      getTimeStepInfo(line, this->Delimeter[0], this->Info->MetaLookUp, timeInfo);
      double time = -1;
      bool valid = convert(timeInfo["time"], time);
      if (valid)
      {
        file_stream.seekg(peakLineG);
      }
    }
    else
    {
      // normal data
      getTimeStepInfo(line, this->Delimeter[0], this->Info->MetaLookUp, timeInfo);
      TimeStep step;
      step.file_pos = tellgValue;
      convert(timeInfo["time"], step.time);
      this->Info->TimeSteps.push_back(step);
    }
    ++row;
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    int size = static_cast<int>(this->Info->TimeSteps.size());

    // set the values at all the time steps
    double* times = new double[size];
    for (int i = 0; i < size; ++i)
    {
      times[i] = this->Info->TimeSteps[i].time;
    }

    // set the time range

    double timeRange[3];
    timeRange[0] = this->Info->TimeSteps[0].time;
    timeRange[1] = this->Info->TimeSteps[size - 1].time;

    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times, size);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    delete[] times;
  }
  file_stream.close();
  return 1;
}

//-------- ---------------------------------------------------------------------
int vtkSpyPlotHistoryReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  if (this->CachedOutput == NULL)
  {
    // fill the output on the first request
    this->CachedOutput = new CachedTables();
    this->FillCache();
  }
  int tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  int TimeIndex = 0;
  // Check if a particular time was requested by the pipeline.
  // This overrides the ivar.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && tsLength > 0)
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // find the first time value larger than requested time value
    // this logic could be improved
    while (TimeIndex < tsLength - 1 && steps[TimeIndex] < requestedTimeStep)
    {
      TimeIndex++;
    }
  }
  output->ShallowCopy(this->CachedOutput->Tables[TimeIndex]);
  return 1;
}
//-----------------------------------------------------------------------------
void vtkSpyPlotHistoryReader::FillCache()
{
  std::string line;
  std::ifstream file_stream(this->FileName, std::ifstream::in);

  std::vector<TimeStep>::iterator tsIt;
  for (tsIt = this->Info->TimeSteps.begin(); tsIt != this->Info->TimeSteps.end(); tsIt++)
  {
    file_stream.seekg(tsIt->file_pos);
    getline(file_stream, line);

    // construct the table
    vtkTable* output = vtkTable::New();
    this->ConstructTableColumns(output);

    // split the line into the items we want to add into the table
    std::vector<std::string> items;
    items.reserve(line.size() / 2);
    split(line, this->Delimeter[0], items);

    // determine the number of rows our table will have
    vtkIdType numRows = static_cast<vtkIdType>(this->Info->ColumnIndexToTracerId.size());
    output->SetNumberOfRows(numRows);

    // setup variables we need in the while loop
    vtkFieldData* fa = output->GetFieldData();
    vtkIdType numCols = output->GetNumberOfColumns();
    std::vector<std::string>::const_iterator it(items.begin());
    int index = 0;
    double tempValue = 0;
    vtkIdType i = 0, j = 0;
    while (it != items.end())
    {
      if (this->Info->ColumnIndexToTracerId.count(index))
      {
        // add in the tracer id first
        output->SetValue(i, 0, vtkVariant(this->Info->ColumnIndexToTracerId[index]));
        for (j = 1; j < numCols; ++j)
        {
          output->SetValue(i, j, vtkVariant(*it));
          ++it;
          ++index;
        }
        ++i;
        continue;
      }
      else if (this->Info->FieldIndexesToNames.find(index) != this->Info->FieldIndexesToNames.end())
      {
        // we have field data
        vtkDoubleArray* fieldData = vtkDoubleArray::New();
        fieldData->SetName((this->Info->FieldIndexesToNames[index]).c_str());
        fieldData->SetNumberOfValues(1);
        convert(*it, tempValue);
        fieldData->InsertValue(0, tempValue);
        fa->AddArray(fieldData);
        fieldData->FastDelete();
      }
      ++it;
      ++index;
    }
    this->CachedOutput->Tables.push_back(output);
  }
  file_stream.close();
}

//-----------------------------------------------------------------------------
void vtkSpyPlotHistoryReader::ConstructTableColumns(vtkTable* table)
{
  std::vector<std::string>::const_iterator hIt;

  // add in the tracer_id column
  vtkIdTypeArray* tracerIdCol = vtkIdTypeArray::New();
  tracerIdCol->SetName("TracerID");
  table->AddColumn(tracerIdCol);
  tracerIdCol->FastDelete();

  for (hIt = this->Info->Header.begin(); hIt != this->Info->Header.end(); ++hIt)
  {
    vtkDoubleArray* col = vtkDoubleArray::New();
    col->SetName((*hIt).c_str());
    table->AddColumn(col);
    col->FastDelete();
  }
}
