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
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkSmartPointer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <iostream>
#include <sstream>
#include <fstream>


using namespace SpyPlotHistoryReaderPrivate;
vtkStandardNewMacro(vtkSpyPlotHistoryReader);

//-----------------------------------------------------------------------------
//meta information of the file
class vtkSpyPlotHistoryReader::MetaInfo
{
public:
  MetaInfo()
    {
    timeSteps.reserve(1024);
    metaIndexes["time"]=-1;
    }
  ~MetaInfo()
    {
    }
  std::map<std::string,int> metaIndexes;
  std::map<int,std::string> metaLookUp;
  std::map<int,int> headerRowToIndex;

  std::vector<TimeStep> timeSteps;
  std::string header;

};

//-----------------------------------------------------------------------------
vtkSpyPlotHistoryReader::vtkSpyPlotHistoryReader():
Info(new MetaInfo)
{
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
}

//----------------------------------------------------------------------------
void vtkSpyPlotHistoryReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Read the file to gather the number of timesteps
int vtkSpyPlotHistoryReader::RequestInformation(vtkInformation *request,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  vtkInformation *info=outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);


  // Open the file and get the number time steps
  std::string line;
  std::ifstream file_stream ( this->FileName , std::ifstream::in );
  int row=0;
  while(file_stream.good())
    {
    getline(file_stream,line);

    //skip any line that starts with a comment
    if (line[0] == this->CommentCharacter[0])
      {
      continue;
      }
    if (row==0)
      {
      //read the header
      //now find the cycle and time information and store those indexes
      getMetaHeaderInfo(line,this->Delimeter[0],this->Info->metaIndexes,
                        this->Info->metaLookUp);

      //now convert this line into a table
      //we are going to have to reduce the header collection
      this->Info->header = createTableLayoutFromHeader(
          line,this->Delimeter[0],this->Info->headerRowToIndex);

      //skip the next line it is junk info
      //this needs to be smarter and optional
      getline(file_stream,line);
      }
    else
      {
      //normal data
      std::vector<std::string> info = getTimeStepInfo(line,
                                  this->Delimeter[0],this->Info->metaLookUp);
      TimeStep step;
      step.file_pos = file_stream.tellg();
      convert(info[this->Info->metaIndexes["time"]],step.time);
      this->Info->timeSteps.push_back(step);
      }
    ++row;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    int size = static_cast<int>(this->Info->timeSteps.size());

    //set the values at all the time steps
    double *times = new double[size];
    for (int i=0; i < size; ++i)
      {
      times[i] = this->Info->timeSteps[i].time;
      }

    //set the time range
    double timeRange[3];
    timeRange[0] = this->Info->timeSteps[0].time;
    timeRange[1] = this->Info->timeSteps[size-1].time;

    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 times,size);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 timeRange, 2);
    delete[] times;
  }
  file_stream.close();
  return 1;
}


//-----------------------------------------------------------------------------
int vtkSpyPlotHistoryReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int tsLength =
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double TimeStepValue = 0;
  unsigned int TimeIndex = 0;
  // Check if a particular time was requested by the pipeline.
  // This overrides the ivar.
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()) && tsLength>0)
    {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double *requestedTimeSteps =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

    // find the first time value larger than requested time value
    // this logic could be improved
    while (TimeIndex < tsLength-1 && steps[TimeIndex] < requestedTimeSteps[0])
      {
      TimeIndex++;
      }
    }

  std::string line;
  std::ifstream file_stream ( this->FileName , std::ifstream::in );
  file_stream.seekg(this->Info->timeSteps[TimeIndex].file_pos);
  getline(file_stream,line);

  //convert the line into the table.
  vtkTable *output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //setup the cols
  this->ConstructTableColumns(output);

  //fill the table
  std::vector<std::string> items;

  items.reserve(line.size()/2);
  split(line,this->Delimeter[0],items);

  int numRows = this->Info->headerRowToIndex.size();
  output->SetNumberOfRows(numRows);

  int numCols = output->GetNumberOfColumns();
  std::vector<std::string>::const_iterator it(items.begin());
  it += this->Info->headerRowToIndex[0];

  for (size_t i= 0; i < numRows; ++i)
    {
    for(size_t j=0; j < numCols; ++j)
      {
      output->SetValue(i,j,vtkVariant(*it));
      ++it;
      }
    }

  file_stream.close();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotHistoryReader::ConstructTableColumns(
  vtkTable *table)
{
  std::vector<std::string> colNames;
  std::vector<std::string>::const_iterator colIt;
  colNames.reserve(this->Info->header.size()/2);
  split(this->Info->header,this->Delimeter[0],colNames);

  for(colIt = colNames.begin();colIt != colNames.end(); ++colIt)
    {
    vtkStringArray *col = vtkStringArray::New();
    col->SetName((*colIt).c_str());
    table->AddColumn(col);
    col->FastDelete();
    }
}
