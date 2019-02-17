/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkGenIOReader.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnstructuredGrid.h"

#include "GIO/GenericIO.h"
#include "utils/timer.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <thread>

/*
#ifndef LANL_GENERICIO_NO_MPI
#include <mpi.h>
#endif

#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <set>
#include <sstream>
#include <unordered_map>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <vtkCellArray.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMultiProcessController.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkType.h>
#include <vtkTypeInt16Array.h>
#include <vtkTypeInt32Array.h>
#include <vtkTypeInt64Array.h>
#include <vtkTypeInt8Array.h>
#include <vtkTypeUInt16Array.h>
#include <vtkTypeUInt32Array.h>
#include <vtkTypeUInt64Array.h>
#include <vtkTypeUInt8Array.h>
#include <vtkUnstructuredGrid.h>

#include "vtkDataArraySelection.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include "LANL/GIO/GenericIO.h"

#include "LANL/utils/timer.h"
*/

vtkStandardNewMacro(vtkGenIOReader)

  vtkGenIOReader::vtkGenIOReader()
{
  this->Controller = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // data
  gioReader = NULL;
  numDataRanks = 0;
  numVars = 0;
  metaDataBuilt = false;

  // sampling
  sampleType = 0; // full data

  // % loading
  dataPercentage = 0.1;
  percentageType = 1; // 0:normal, 1:power cube

  // Selections
  selectionChanged = false;
  randomSeed = std::chrono::system_clock::now().time_since_epoch().count();
  CellDataArraySelection = vtkDataArraySelection::New();

  // Timeseries
  justLoaded = true;

  // MPI
  InitMPICommunicator();

  // Threads
  concurentThreadsSupported = std::max(1, (int)std::thread::hardware_concurrency());
  randomNumGenerated = false;

  // Log
  debugLog.setOutputFilename(
    "paraviewCosmo_" + std::to_string(myRank) + "_of_" + std::to_string(numRanks) + ".log");

  msgLog << "#threads to launch: " << concurentThreadsSupported << std::endl;
  msgLog << "Leaving constructor ...\n" << std::endl;
  debugLog.writeLogToDisk(msgLog);
}

vtkGenIOReader::~vtkGenIOReader()
{
  if (gioReader)
  {
    gioReader->close();
    delete gioReader;
    gioReader = NULL;
  }

  CellDataArraySelection->Delete();
  CellDataArraySelection = 0;
}

//
// UI
void vtkGenIOReader::SetFileName(char* fname)
{
  dataFilename = std::string(fname);
  msgLog << "SetFileName | Opening filename: " << dataFilename << " ...\n";

  this->Modified();
}

void vtkGenIOReader::SetSampleType(int s)
{
  if (sampleType != s)
  {
    sampleType = s;
    this->Modified();
  }
}

void vtkGenIOReader::SetDataPercentToShow(double t)
{
  if (dataPercentage != t)
  {
    dataPercentage = t;
    dataNumShowElements = dataPercentage * totalNumberOfElements;
    this->Modified();
  }
}

void vtkGenIOReader::SetPercentageType(int _type)
{
  if (percentageType != _type)
  {
    percentageType = _type;
    this->Modified();
  }
}

void vtkGenIOReader::SetResetSelection(int /* _x */)
{
  selections.clear();
  selectionChanged = true;
  this->Modified();
}

void vtkGenIOReader::SelectScalar(const char* selectedScalar)
{
  std::string _selectedScalar = std::string(selectedScalar);
  if (_sel.selectedScalar != _selectedScalar)
  {
    _sel.selectedScalar = std::string(_selectedScalar);
    selectionChanged = true;
    this->Modified();
  }
}

void vtkGenIOReader::SelectCriteria(int selectionCriteria)
{
  if (_sel.operatorType != selectionCriteria)
  {
    _sel.operatorType = selectionCriteria;
    selectionChanged = true;
    this->Modified();
  }
}

void vtkGenIOReader::SelectValue1(const char* value1)
{
  std::string _value1 = std::string(value1);
  if (_sel.selectedValue[0] != _value1)
  {
    _sel.selectedValue[0] = std::string(value1);
    selectionChanged = true;
    this->Modified();
  }
}

void vtkGenIOReader::SelectValue2(const char* value2)
{
  std::string _value2 = std::string(value2);
  if (_sel.selectedValue[1] != _value2)
  {
    _sel.selectedValue[1] = std::string(value2);
    selectionChanged = true;
    this->Modified();
  }
}

//
// Utilities
void vtkGenIOReader::SetCellArrayStatus(const char* name, int status)
{
  if (CellDataArraySelection->ArrayIsEnabled(name) == (status ? 1 : 0))
    return;

  if (status)
    CellDataArraySelection->EnableArray(name);
  else
    CellDataArraySelection->DisableArray(name);

  this->Modified();
}

void vtkGenIOReader::InitMPICommunicator()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();

  myRank = this->Controller->GetLocalProcessId();
  numRanks = this->Controller->GetNumberOfProcesses();

  msgLog << "myRank: " << myRank << ", num ranks:" << numRanks << "\n";
}

void vtkGenIOReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File: " << (this->dataFilename.c_str() ? this->dataFilename.c_str() : "none")
     << "\n";
}

void vtkGenIOReader::displayMsg(std::string msg)
{
  char* cstr = new char[msg.length() + 1];
  strcpy(cstr, msg.c_str());
  vtkOutputWindowDisplayText(cstr);
}

bool vtkGenIOReader::doMPIDataSplitting(int numDataRanksTmp, int numMPIranks, int myRankTmp,
  int ranksRangeToLoad[2], std::vector<size_t>& readRowsInfo)
{
  bool splitReading = false;
  if (numDataRanksTmp >= numMPIranks)
  {
    int eachMPIRanksLoads = floor(numDataRanksTmp / (float)numMPIranks); // 256/10 = 25.6 = 25
    int sumFloorAllocation = numDataRanksTmp - eachMPIRanksLoads * numMPIranks; // 256 - 10*25 = 16

    int countAllocation = 0;
    for (int i = 0; i < numMPIranks; i++)
    {
      if (myRankTmp == i)
        ranksRangeToLoad[0] = countAllocation;

      countAllocation += eachMPIRanksLoads;
      if (i < sumFloorAllocation)
        countAllocation++;

      if (myRankTmp == i)
        ranksRangeToLoad[1] = countAllocation - 1;
    }
    msgLog << "More data ranks than MPI ranks | My rank: " << myRankTmp
           << ", num data ranks: " << numDataRanksTmp << ", read extents: " << ranksRangeToLoad[0]
           << " - " << ranksRangeToLoad[1] << "\n";
  }
  else
  {
    splitReading = true;
    double eachMPIRanksLoads = numDataRanksTmp / (double)numMPIranks;

    double startFraction = eachMPIRanksLoads * myRankTmp;
    double endFraction = startFraction + eachMPIRanksLoads;

    ranksRangeToLoad[0] = std::max(std::min((int)startFraction, numDataRanksTmp - 1), 0);
    ranksRangeToLoad[1] = std::max(std::min((int)endFraction, numDataRanksTmp - 1), 0);

    msgLog << "numDataRanks: " << numDataRanksTmp << "   numRanks: " << numMPIranks
           << "   eachMPIRanksLoads: " << eachMPIRanksLoads << "\n";
    msgLog << "ranksRangeToLoad[0]: " << ranksRangeToLoad[0]
           << "   ranksRangeToLoad[1]: " << ranksRangeToLoad[1] << "\n";
    msgLog << "startFraction: " << startFraction << "   endFraction: " << endFraction << "\n";

    if (ranksRangeToLoad[0] == ranksRangeToLoad[1])
    {
      size_t Np = gioReader->readNumElems(ranksRangeToLoad[0]);
      msgLog << "Np: " << Np << "\n";
      size_t startRow = (startFraction - ranksRangeToLoad[0]) * Np;
      size_t endRow = (endFraction - ranksRangeToLoad[0]) * Np;

      readRowsInfo.push_back(ranksRangeToLoad[0]);
      readRowsInfo.push_back(startRow);
      readRowsInfo.push_back(endRow - startRow);
    }
    else
    {
      size_t Np = gioReader->readNumElems(ranksRangeToLoad[0]);
      msgLog << "Np: " << Np << "\n";

      size_t startRow = (startFraction - ranksRangeToLoad[0]) * Np;

      readRowsInfo.push_back(ranksRangeToLoad[0]);
      readRowsInfo.push_back(startRow);
      readRowsInfo.push_back(Np - startRow);

      msgLog << "ranksRangeToLoad[0]: " << readRowsInfo[0] << "\n";
      msgLog << "startRow: " << readRowsInfo[1] << "\n";
      msgLog << "Np-startRow: " << readRowsInfo[2] << "\n";

      Np = gioReader->readNumElems(ranksRangeToLoad[1]);
      size_t endRow = (endFraction - (int)endFraction) * Np;

      readRowsInfo.push_back(ranksRangeToLoad[1]);
      readRowsInfo.push_back(0);
      readRowsInfo.push_back(endRow);

      msgLog << "ranksRangeToLoad[1]: " << readRowsInfo[3 + 0] << "\n";
      msgLog << "startRow: " << readRowsInfo[3 + 1] << "\n";
      msgLog << "endRow: " << readRowsInfo[3 + 2] << "\n";
    }

    for (size_t i = 0; i < readRowsInfo.size(); i += 3)
      msgLog << "Split done! | My rank: " << myRankTmp << " : " << readRowsInfo[i + 0] << ", "
             << readRowsInfo[i + 1] << ", " << readRowsInfo[i + 2] << "\n";
  }

  debugLog.writeLogToDisk(msgLog);
  return splitReading;
}

void vtkGenIOReader::theadedParsing(int threadId, int numThreads, size_t numRowsToSample,
  size_t numLoadingRows, vtkSmartPointer<vtkCellArray> cells, vtkSmartPointer<vtkPoints> pnts,
  int numSelections)
{
  GIOPvPlugin::Timer parseClock;

  // parseClock.start();
  vtkIdType _idx;

  size_t rowsPerThread = floor(numLoadingRows / (float)numThreads);
  size_t startRow = rowsPerThread * threadId;
  size_t numRowsToSamplePerThread = floor(numRowsToSample / (float)numThreads);
  if (threadId == numThreads - 1)
  {
    size_t count = (numThreads - 1) * numRowsToSamplePerThread;
    numRowsToSamplePerThread = numRowsToSample - count;
    if (startRow + numRowsToSamplePerThread > numLoadingRows)
      numRowsToSamplePerThread = numLoadingRows - startRow;
  }

  double pnt[3];
  for (size_t j = startRow; j < (startRow + numRowsToSamplePerThread); ++j)
  {
    // Choose random element to load
    size_t _j = _num[j];
    while (_j >= numLoadingRows)
    {
      mtx.lock();
      size_t _nextHash = nextHash;
      nextHash++;
      mtx.unlock();

      _j = _num[_nextHash];
    }

    //
    // Selection
    if (numSelections != -1)
    {
      bool matchedCriteria = true;
      for (int i = 0; i < numSelections; i++)
      {
        for (size_t k = 0; k < paraviewData.size(); k++)
        {
          ParaviewSelection __sel = selections[i];

          std::string _name = readInData[k].name;
          if (_name == __sel.selectedScalar)
          {
            bool localMatchedCriteria = false;
            std::string _dataType = readInData[k].dataType;

            if (__sel.operatorType == 0)
              localMatchedCriteria = readInData[k].equal(__sel.selectedValue[0], _dataType, _j);
            else if (__sel.operatorType == 1)
              localMatchedCriteria =
                readInData[k].greaterEqual(__sel.selectedValue[0], _dataType, _j);
            else if (__sel.operatorType == 2)
              localMatchedCriteria = readInData[k].lessEqual(__sel.selectedValue[0], _dataType, _j);
            else if (__sel.operatorType == 3)
              localMatchedCriteria = readInData[k].isBetween(
                __sel.selectedValue[0], __sel.selectedValue[1], _dataType, _j);

            matchedCriteria = matchedCriteria && localMatchedCriteria;
            if (!matchedCriteria)
              break;
          }
        }

        if (!matchedCriteria)
          break;
      }
      if (!matchedCriteria)
        continue;
    }

    mtx.lock();
    _idx = idx;
    idx++;
    mtx.unlock();

    int tupleCount = 0;
    for (size_t k = 0; k < paraviewData.size(); k++)
    {
      // Load the x,y,z variables
      if (paraviewData[k].xVar)
        pnt[0] = ((float*)readInData[k].data)[_j];

      if (paraviewData[k].yVar)
        pnt[1] = ((float*)readInData[k].data)[_j];

      if (paraviewData[k].zVar)
        pnt[2] = ((float*)readInData[k].data)[_j];

      // Load the scalars that the user wants to see
      if (paraviewData[k].show)
      {
        std::string _dataType = readInData[k].dataType;

        mtx.lock();
        if (_dataType == "float")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((float*)readInData[k].data)[_j]);
        else if (_dataType == "double")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((double*)readInData[k].data)[_j]);
        else if (_dataType == "int8_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((int8_t*)readInData[k].data)[_j]);
        else if (_dataType == "int16_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((int16_t*)readInData[k].data)[_j]);
        else if (_dataType == "int32_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((int32_t*)readInData[k].data)[_j]);
        else if (_dataType == "int64_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((int64_t*)readInData[k].data)[_j]);
        else if (_dataType == "uint8_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((uint8_t*)readInData[k].data)[_j]);
        else if (_dataType == "uint16_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((uint16_t*)readInData[k].data)[_j]);
        else if (_dataType == "uint32_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((uint32_t*)readInData[k].data)[_j]);
        else if (_dataType == "uint64_t")
          (tupleArray[tupleCount])->InsertTuple1(_idx, ((uint64_t*)readInData[k].data)[_j]);
        else
          msgLog << _dataType << " ...data type undefined !!!";
        mtx.unlock();

        tupleCount++;
      }
    }

    mtx.lock();
    pnts->InsertPoint(_idx, pnt);
    cells->InsertNextCell(1, &_idx);
    totalPoints++;
    mtx.unlock();
  }

  // parseClock.stop();
  // msgLog <<" time taken ~ parsing for thread id "  << threadId << " took " <<
  // parseClock.getDuration() << " s.\n";
}

//
// Core components
int vtkGenIOReader::RequestInformation(vtkInformation* /*rqst*/,
  vtkInformationVector** /*inputVector*/, vtkInformationVector* outputVector)
{
  GIOPvPlugin::Timer fullClock, resampledClock, haloClock;
  msgLog << "\nRequestInformation for: " << dataFilename << "\n";

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  fullClock.start();
  if (gioReader != NULL)
  {
    if (currentFilename != dataFilename)
    {
      msgLog << "currentFilename: " << currentFilename << ", dataFilename: " << dataFilename
             << "\n";
      gioReader->close();
      delete gioReader;
      gioReader = NULL;

      metaDataBuilt = false; // signal to re-build metadata
      unsigned Method = lanl::gio::GenericIO::FileIOPOSIX;

      randomNumGenerated = false;
      gioReader = new lanl::gio::GenericIO(dataFilename.c_str(), Method);
      msgLog << "Opening ... \n";

      currentFilename = dataFilename;
      msgLog << "gioReader != NULL\n";
    }
  }
  else
  {
    metaDataBuilt = false; // signal to re-build metadata
    unsigned Method = lanl::gio::GenericIO::FileIOPOSIX;

    randomNumGenerated = false;
    gioReader = new lanl::gio::GenericIO(dataFilename.c_str(), Method);
    msgLog << "Opening . .. .\n";

    currentFilename = dataFilename;
    msgLog << "gioReader == NULL\n";
  }

  assert("post: Internal GenericIO reader should not be NULL!" && (this->gioReader != NULL));

  if (!metaDataBuilt)
  {
    gioReader->openAndReadHeader(lanl::gio::GenericIO::MismatchRedistribute);
    msgLog << "header opened ... reading vars ... \n";

    totalNumberOfElements = 0;
    numDataRanks = this->gioReader->readNRanks();
    msgLog << "numDataRanks: " << numDataRanks << "\n";
    for (int i = 0; i < numDataRanks; ++i)
      totalNumberOfElements += this->gioReader->readNumElems(i);

    std::vector<lanl::gio::GenericIO::VariableInfo> VI;
    gioReader->getVariableInfo(VI);

    numVars = static_cast<int>(VI.size());
    readInData.resize(numVars);

    bool foundCoord = false;
    for (int i = 0; i < numVars; i++)
    {
      readInData[i].id = i;
      readInData[i].name = VI[i].Name;
      readInData[i].size = static_cast<int>(VI[i].Size);
      readInData[i].isFloat = VI[i].IsFloat;
      readInData[i].isSigned = VI[i].IsSigned;
      readInData[i].ghost = VI[i].MaybePhysGhost;
      readInData[i].xVar = VI[i].IsPhysCoordX;
      readInData[i].yVar = VI[i].IsPhysCoordY;
      readInData[i].zVar = VI[i].IsPhysCoordZ;
      readInData[i].determineDataType();

      if (justLoaded)
      {
        CellDataArraySelection->AddArray((VI[i].Name).c_str());
        SetCellArrayStatus((VI[i].Name).c_str(), 1);
      }
      else
        SetCellArrayStatus((VI[i].Name).c_str(), GetCellArrayStatus((VI[i].Name).c_str()));

      if (readInData[i].xVar)
        foundCoord = true;

      msgLog << std::to_string(i) << " : " << readInData[i].name << ", " << readInData[i].size
             << ", " << readInData[i].isFloat << ", " << readInData[i].isSigned << ", "
             << readInData[i].ghost << ", " << readInData[i].xVar << ", " << readInData[i].yVar
             << ", " << readInData[i].zVar << "\n";
    }

    // Some GIO files have positional information but
    // do not indicate it! So try to manually find that!
    if (!foundCoord)
    {
      bool foundX, foundY, foundZ;
      foundX = foundY = foundZ = false;
      for (int i = 0; i < numVars; i++)
      {
        if (!foundX)
        {
          std::size_t _found = (readInData[i].name).find("_x");
          if (_found != std::string::npos)
          {
            readInData[i].xVar = true;
            foundX = true;
          }
        }

        if (!foundY)
        {
          std::size_t _found = (readInData[i].name).find("_y");
          if (_found != std::string::npos)
          {
            readInData[i].yVar = true;
            foundY = true;
          }
        }

        if (!foundZ)
        {
          std::size_t _found = (readInData[i].name).find("_z");
          if (_found != std::string::npos)
          {
            readInData[i].zVar = true;
            foundZ = true;
          }
        }
      }

      if ((foundZ && foundY) && foundX)
        foundCoord = true;
    }

    if (!foundCoord)
    {
      bool foundX, foundY, foundZ;
      foundX = foundY = foundZ = false;
      for (int i = 0; i < numVars; i++)
      {
        if (!foundX)
        {
          if (readInData[i].name == "x")
          {
            readInData[i].xVar = true;
            foundX = true;
          }
        }

        if (!foundY)
        {
          if (readInData[i].name == "y")
          {
            readInData[i].yVar = true;
            foundY = true;
          }
        }

        if (!foundZ)
        {
          if (readInData[i].name == "z")
          {
            readInData[i].zVar = true;
            foundZ = true;
          }
        }
      }
    }

    //
    // Determine tuples
    paraviewData.clear();

    bool xVarFound, yVarFound, zVarFound;
    xVarFound = yVarFound = zVarFound = false;
    for (int i = 0; i < numVars; i++)
    {
      ParaviewField _temp(readInData[i].name);

      if (readInData[i].xVar && !xVarFound)
      {
        _temp.position = _temp.xVar = true;
        xVarFound = true;
        msgLog << "x var: " << readInData[i].name << "\n";
      }

      if (readInData[i].yVar && !yVarFound)
      {
        _temp.position = _temp.yVar = true;
        yVarFound = true;
        msgLog << "y var: " << readInData[i].name << "\n";
      }

      if (readInData[i].zVar && !zVarFound)
      {
        _temp.position = _temp.zVar = true;
        zVarFound = true;
        msgLog << "z var: " << readInData[i].name << "\n";
      }

      paraviewData.push_back(_temp);
    }

    metaDataBuilt = true;
    msgLog << "numVars: " + std::to_string(numVars) << "\n";
  }
  else
    msgLog << "\nHeader file already opened!\n";

  fullClock.stop();

  msgLog << "\nTiming:\n";
  msgLog << "   Header full reading: " << fullClock.getDuration() << " s.\n\n";
  msgLog << "======================================================================================"
            "=\n\n";
  debugLog.writeLogToDisk(msgLog);

  justLoaded = false;

  return 1;
}

int vtkGenIOReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  GIOPvPlugin::Timer setupClock, dataReadingClock, populatingClock, cleanupClock, intializeClock,
    readClock, _clock, loadClock, parseClock, hashClock;
  msgLog << "\nRequestData for: " << dataFilename << "...\n";
  msgLog << "\nRequestData - Total # of rows: " << totalNumberOfElements << "\n";

  intializeClock.start();
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

#if 0 // Not used.
  int piece, numPieces;
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
#endif

  //
  // Selection
  int numSelections = 0;
  if (sampleType == 3 && selectionChanged == true)
  {
    if (selections.size() == 0)
      selections.push_back(_sel);
    else
    {
      int numEntriesInSel = static_cast<int>(selections.size());
      if (!((_sel.selectedScalar == selections[numEntriesInSel - 1].selectedScalar &&
              _sel.operatorType == selections[numEntriesInSel - 1].operatorType) &&
            (_sel.selectedValue[0] == selections[numEntriesInSel - 1].selectedValue[0] &&
              _sel.selectedValue[1] == selections[numEntriesInSel - 1].selectedValue[1])))
        selections.push_back(_sel);
    }

    numSelections = static_cast<int>(selections.size());

    msgLog << "numSelections: " << numSelections << "\n";
    for (int i = 0; i < numSelections; i++)
      msgLog << "name, val: " << selections[i].selectedScalar << ", "
             << selections[i].selectedValue[0] << "\n";

    selectionChanged = false;
  }

  //
  // Determines what needs to be loaded and shown
  int numActiveTuples = 0;
  for (int i = 0; i < numVars; i++)
  {
    const char* _name = CellDataArraySelection->GetArrayName(i);
    int _status = GetCellArrayStatus(_name);

    paraviewData[i].show = _status != 0;
    paraviewData[i].load = _status != 0;

    // override above if it's a position scalar
    if (paraviewData[i].position)
      paraviewData[i].load = 1;

    msgLog << "Var: " + std::string(_name) << " ~ show: " << paraviewData[i].show
           << " ~ load: " << paraviewData[i].load << "\n";
  }

  //
  // Split data reading
  bool splitReading;
  int ranksRangeToLoad[2];
  std::vector<size_t> readRowsInfo; // (rank, start row, num rows)
  splitReading = doMPIDataSplitting(numDataRanks, numRanks, myRank, ranksRangeToLoad, readRowsInfo);

  //
  // Adjust based on the percentage of data we want to show
  size_t maxRowsInRank = 0;
  int splitReadingCount = 0;
  for (int i = ranksRangeToLoad[0]; i <= ranksRangeToLoad[1]; ++i)
  {
    size_t numLoadingRows;
    if (!splitReading)
      numLoadingRows = gioReader->readNumElems(i);
    else
    {
      numLoadingRows = readRowsInfo[splitReadingCount * 3 + 2];
      splitReadingCount++;
    }
    maxRowsInRank = std::max(maxRowsInRank, numLoadingRows);

// Find the number of rows after sampling
#if 0 // numRowsToSample is unused before scope ends.
    size_t numRowsToSample = numLoadingRows;
    if (percentageType == 0) // normal
      numRowsToSample = numLoadingRows * dataPercentage;
    else
      numRowsToSample = numLoadingRows * (dataPercentage * dataPercentage * dataPercentage);
#endif
  }

  //
  // Generate a random number, sort of hashing really where each key is unique
  if (!randomNumGenerated)
  {
    hashClock.start();
    _num.resize(maxRowsInRank);
    std::iota(_num.begin(), _num.end(), 0);
    shuffle(_num.begin(), _num.end(), std::default_random_engine(randomSeed));
    hashClock.stop();
    randomNumGenerated = true;
    msgLog << " time taken ~ hashing: " << hashClock.getDuration() << " s.\n";
  }
  msgLog << "maxRowsInRank: " << maxRowsInRank << "\n";
  msgLog << "Percentage: " << dataPercentage << ", dataNumShowElements: " << dataNumShowElements
         << "\n";

  //
  // Initialize points and cells
  idx = 0;
  vtkSmartPointer<vtkPoints> pnts = vtkSmartPointer<vtkPoints>::New();
  // pnts = vtkSmartPointer<vtkPoints>::New();
  pnts->SetDataTypeToDouble();
  // cells = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  tupleArray.resize(numVars);

  //
  // Create the vtk structures to show the data
  int tupleCount = 0;
  for (size_t i = 0; i < paraviewData.size(); i++)
  {
    if (paraviewData[i].show)
    {
      std::string _dataType = readInData[i].dataType;

      if (_dataType == "float")
        (tupleArray[tupleCount]) = vtkFloatArray::New();
      else if (_dataType == "double")
        (tupleArray[tupleCount]) = vtkDoubleArray::New();
      else if (_dataType == "int8_t")
        (tupleArray[tupleCount]) = vtkTypeInt8Array::New();
      else if (_dataType == "int16_t")
        (tupleArray[tupleCount]) = vtkTypeInt16Array::New();
      else if (_dataType == "int32_t")
        (tupleArray[tupleCount]) = vtkTypeInt32Array::New();
      else if (_dataType == "int64_t")
        (tupleArray[tupleCount]) = vtkTypeInt64Array::New();
      else if (_dataType == "uint8_t")
        (tupleArray[tupleCount]) = vtkTypeUInt8Array::New();
      else if (_dataType == "uint16_t")
        (tupleArray[tupleCount]) = vtkTypeUInt16Array::New();
      else if (_dataType == "uint32_t")
        (tupleArray[tupleCount]) = vtkTypeUInt32Array::New();
      else if (_dataType == "uint64_t")
        (tupleArray[tupleCount]) = vtkTypeUInt64Array::New();
      else
      {
        msgLog << _dataType << " type not found! Using float as surrogate.\n";
        (tupleArray[tupleCount]) = vtkFloatArray::New();
      }

      (tupleArray[tupleCount])->SetName((paraviewData[i].name).c_str());
      (tupleArray[tupleCount])->SetNumberOfComponents(1);
      tupleCount++;
    }
  }
  numActiveTuples = tupleCount;

  intializeClock.stop();
  msgLog << "\nReading now: " << numActiveTuples << " ... \n";
  debugLog.writeLogToDisk(msgLog);

  totalPoints = 0;
  size_t totalPointsProcessed = 0;
  populatingClock.start();
  switch (this->sampleType)
  {
    case 0:
    {
      msgLog << "\nShow all sampled; sample type = " << std::to_string(this->sampleType) << "\n";

      for (int i = ranksRangeToLoad[0]; i <= ranksRangeToLoad[1]; ++i)
      {
        size_t Np = gioReader->readNumElems(i);
        totalPointsProcessed += Np;

        int Coords[3];
        gioReader->readCoords(Coords, i);

        _clock.start();

        // Specify location where to store each var read in
        for (size_t j = 0; j < readInData.size(); j++)
        {
          if (paraviewData[j].load)
          {
            readInData[j].setNumElements(Np);
            readInData[j].allocateMem(1);

            if (readInData[j].dataType == "float")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (float*)readInData[j].data, true);
            else if (readInData[j].dataType == "double")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (double*)readInData[j].data, true);
            else if (readInData[j].dataType == "int8_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int8_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int16_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int16_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int32_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int32_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int64_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int64_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint8_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint8_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint16_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint16_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint32_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint32_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint64_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint64_t*)readInData[j].data, true);
            else
              msgLog << readInData[j].dataType << " = data type undefined!!!";
          }
        }
        _clock.stop();
        msgLog << "\n\nInput read rank: " << i << ", paraviewData.size(): " << paraviewData.size()
               << ", time to create structures: " << _clock.getDuration() << " s.\n";

        loadClock.start();

        // Load data
        size_t numLoadingRows;
        if (!splitReading)
        {
          gioReader->readDataSection(0, Np, i, false); // reading the whole file
          numLoadingRows = Np;
        }
        else
        {
          gioReader->readDataSection(readRowsInfo[splitReadingCount * 3 + 1],
            readRowsInfo[splitReadingCount * 3 + 2], i, false);
          numLoadingRows = readRowsInfo[splitReadingCount * 3 + 2];
          splitReadingCount++;
        }

        // Find the number of rows after sampling
        size_t numRowsToSample = numLoadingRows;
        if (percentageType == 0) // normal
          numRowsToSample = round(numLoadingRows * dataPercentage);
        else
          numRowsToSample =
            round(numLoadingRows * (dataPercentage * dataPercentage * dataPercentage));

        if (numRowsToSample > numLoadingRows)
          numRowsToSample = numLoadingRows;

        msgLog << "Rank (i): " + std::to_string(i) << ", Np/numLoadingRows: " << numLoadingRows
               << ", # rows in rank: " << gioReader->readNumElems(i)
               << ", dataPercentage: " << dataPercentage
               << ", dataPercentage^3: " << dataPercentage * dataPercentage * dataPercentage
               << ", numRowsToSample: " << numRowsToSample << "\n";
        loadClock.stop();
        msgLog << " time taken ~ loading: " << loadClock.getDuration() << " s.\n";
        // debugLog.writeLogToDisk(msgLog);

        // Parse scalars
        parseClock.start();
        nextHash = numLoadingRows;

        std::vector<std::thread> threadPool;

        for (int t = 0; t < concurentThreadsSupported; t++)
          threadPool.push_back(std::thread(&vtkGenIOReader::theadedParsing, this, t,
            concurentThreadsSupported, numRowsToSample, numLoadingRows, cells, pnts, -1));

        for (auto& th : threadPool)
          th.join();
        parseClock.stop();
        msgLog << " time taken ~ parsing: " << parseClock.getDuration() << " s.\n";

        for (size_t j = 0; j < readInData.size(); j++)
          readInData[j].deAllocateMem();

        gioReader->clearVariables();
      }
    }
      msgLog << "Case 0 done!\n";
      debugLog.writeLogToDisk(msgLog);
      break;

    case 3:
    {
      msgLog << "Selecting based on ... \n";

      // Set selected variable
      numSelections = static_cast<int>(selections.size());
      bool foundSelected = false;
      for (int i = 0; i < numSelections; i++)
      {
        ParaviewSelection __sel = selections[i];
        for (size_t j = 0; j < readInData.size(); j++)
          if (__sel.selectedScalar == paraviewData[j].name)
          {
            msgLog << "Selected: " << paraviewData[j].name << "\n";
            foundSelected = true;
            break;
          }

        if (foundSelected)
          break;
      }

      // Stop if none of the selected scalars exist
      if (!foundSelected)
      {
        msgLog << "Selected scalar: " + _sel.selectedScalar + " not found!\n";
        displayMsg("Nothing matches the selected scalars in this dataset ...!\n");
        break;
      }

      for (int i = ranksRangeToLoad[0]; i <= ranksRangeToLoad[1]; ++i)
      {
        size_t Np = gioReader->readNumElems(i);
        totalPointsProcessed += Np;

        int Coords[3];
        gioReader->readCoords(Coords, i);

        _clock.start();
        for (size_t j = 0; j < readInData.size(); j++)
        {
          if (paraviewData[j].load)
          {
            readInData[j].setNumElements(Np);
            readInData[j].allocateMem(1);

            if (readInData[j].dataType == "float")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (float*)readInData[j].data, true);
            else if (readInData[j].dataType == "double")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (double*)readInData[j].data, true);
            else if (readInData[j].dataType == "int8_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int8_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int16_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int16_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int32_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int32_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "int64_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (int64_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint8_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint8_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint16_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint16_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint32_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint32_t*)readInData[j].data, true);
            else if (readInData[j].dataType == "uint64_t")
              gioReader->addVariable(
                (readInData[j].name).c_str(), (uint64_t*)readInData[j].data, true);
            else
              msgLog << readInData[j].dataType << " = data type undefined!!!";
          }
        }
        _clock.stop();
        msgLog << "Input read rank: " << i << ", paraviewData.size(): " << paraviewData.size()
               << ", time to create structures: " << _clock.getDuration() << " s.\n";

        loadClock.start();

        // Find the number of rows to read
        size_t numLoadingRows;
        if (!splitReading)
        {
          gioReader->readDataSection(0, Np, i, false); // reading the whole file
          numLoadingRows = Np;
        }
        else
        {
          gioReader->readDataSection(readRowsInfo[splitReadingCount * 3 + 1],
            readRowsInfo[splitReadingCount * 3 + 2], i, false);
          numLoadingRows = readRowsInfo[splitReadingCount * 3 + 2];
          splitReadingCount++;
        }
        msgLog << "numLoadingRows: " << numLoadingRows << "\n";

        // Find the number of rows after sampling
        size_t numRowsToSample = numLoadingRows;
        if (percentageType == 0) // normal
          numRowsToSample = round(numLoadingRows * dataPercentage);
        else
          numRowsToSample =
            round(numLoadingRows * (dataPercentage * dataPercentage * dataPercentage));

        if (numRowsToSample > numLoadingRows)
          numRowsToSample = numLoadingRows;
        msgLog << "\ni: " + std::to_string(i) << ", Np: " << numLoadingRows
               << ", # rows in rank: " << gioReader->readNumElems(i)
               << ", dataPercentage: " << dataPercentage
               << ", dataPercentage^3: " << dataPercentage * dataPercentage * dataPercentage
               << ", numRowsToSample: " << numRowsToSample;
        loadClock.stop();
        msgLog << " time taken ~ loading: " << loadClock.getDuration() << " s.\n";

        // Load scalars
        parseClock.start();
        nextHash = numLoadingRows;

        std::vector<std::thread> threadPool;

        for (int t = 0; t < concurentThreadsSupported; t++)
          threadPool.push_back(
            std::thread(&vtkGenIOReader::theadedParsing, this, t, concurentThreadsSupported,
              numRowsToSample, numLoadingRows, cells, pnts, numSelections));

        for (auto& th : threadPool)
          th.join();

        parseClock.stop();
        msgLog << " time taken: " << parseClock.getDuration() << " s.\n";

        //
        // Cleanup
        for (size_t j = 0; j < readInData.size(); j++)
          readInData[j].deAllocateMem();

        gioReader->clearVariables();
      }
    }
      msgLog << "Case 3 done\n";
      debugLog.writeLogToDisk(msgLog);
      break;

    default:
      break;
  };
  populatingClock.stop();

  cleanupClock.start();

  output->SetPoints(pnts);
  output->SetCells(VTK_VERTEX, cells);

  for (int i = 0; i < numActiveTuples; i++)
    output->GetPointData()->AddArray(tupleArray[i]);

  output->Squeeze();

  for (int i = 0; i < numActiveTuples; i++)
    (tupleArray[i])->Delete();

  gioReader->clearVariables();

  cleanupClock.stop();

  msgLog << "\ntotalPoints " << totalPoints << " out of " << totalPointsProcessed << "\n";
  msgLog << "numActiveTuples: " << numActiveTuples << "\n";

  msgLog << "\nTiming:\n";
  msgLog << "   Initializing: " + std::to_string(intializeClock.getDuration()) + " s.\n";
  msgLog << "   Populating  : " + std::to_string(populatingClock.getDuration()) + " s.\n";
  msgLog << "   Cleanup     : " + std::to_string(cleanupClock.getDuration()) + " s.\n\n";
  msgLog
    << "=======================================================================================\n";
  msgLog
    << "=======================================================================================\n";

  debugLog.writeLogToDisk(msgLog);

  return 1;
}
