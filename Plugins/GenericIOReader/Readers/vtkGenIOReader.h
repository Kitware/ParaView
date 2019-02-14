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

#ifndef vtkGenIOReader_h
#define vtkGenIOReader_h

#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkGenericIOReaderModule.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include "utils/gioData.h"
#include "utils/log.h"

#include <mutex>
#include <sstream>
#include <string>
#include <vector>

class vtkDataArray;
class vtkMultiProcessController;

namespace lanl
{
namespace gio
{
class GenericIO;
}
}

//
// Creates a selection
struct ParaviewSelection
{
  std::string selectedScalar;
  int operatorType; // 0:is, 1:>=, 2:<=, 3 is between
  std::string selectedValue[2];

  ParaviewSelection()
  {
    selectedScalar = selectedValue[0] = selectedValue[1] = "";
    operatorType = 0;
  }
};

//
// Creates the link between GiO data read in + what paraview shows
struct ParaviewField
{
  std::string name;
  bool show;
  bool load;
  bool position;
  bool xVar, yVar, zVar;
  bool ghost;
  bool query;

  ParaviewField()
  {
    load = show = true;
    position = xVar = yVar = zVar = false;
    query = ghost = false;
  }
  ParaviewField(std::string _name)
    : name(_name)
  {
    load = show = true;
    position = xVar = yVar = zVar = false;
    query = ghost = false;
  }
};

class VTKGENERICIOREADER_EXPORT vtkGenIOReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkGenIOReader* New();
  vtkTypeMacro(vtkGenIOReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //
  // UI methods
  void SetFileName(char* fname);
  void SetSampleType(int s);
  void SetDataPercentToShow(double t);
  void SetPercentageType(int _type);

  void SetResetSelection(int _x);
  void SelectScalar(const char* selectedScalar);
  void SelectCriteria(int selectionCriteria);
  void SelectValue1(const char* value1);
  void SelectValue2(const char* value2);

  //
  // MPI Stuff
  void InitMPICommunicator();

  //
  // Cell array selection
  int GetNumberOfCellArrays() { return CellDataArraySelection->GetNumberOfArrays(); }
  const char* GetCellArrayName(int i) { return CellDataArraySelection->GetArrayName(i); }
  int GetCellArrayStatus(const char* name) { return CellDataArraySelection->ArrayIsEnabled(name); }
  void SetCellArrayStatus(const char* name, int status);

protected:
  vtkGenIOReader();
  ~vtkGenIOReader();

  bool doMPIDataSplitting(int numDataRanks, int numMPIranks, int myRank, int ranksRangeToLoad[2],
    std::vector<size_t>& readRowsInfo);
  int RequestInformation(vtkInformation* rqst, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void theadedParsing(int threadId, int numThreads, size_t numRowsToSample, size_t Np,
    vtkSmartPointer<vtkCellArray> cells, vtkSmartPointer<vtkPoints> pnts, int numSelections = -1);

  void displayMsg(std::string msg);

private:
  // MPI Stuff
  vtkMultiProcessController* Controller;
  int numRanks, myRank;

  // Threads
  std::mutex mtx;
  int concurentThreadsSupported;

  // Sampling type
  int sampleType; // 0:full data, 2:octree(unused) 3:selection

  // Loading
  int percentageType; // 0:normal, 1:power cubelog
  double dataPercentage;
  size_t dataNumShowElements;
  unsigned randomSeed;

  // Selection
  bool selectionChanged;
  ParaviewSelection _sel;
  std::vector<ParaviewSelection> selections;

  // Cell array selection
  vtkDataArraySelection* CellDataArraySelection;

  // GenericIO Data
  lanl::gio::GenericIO* gioReader;
  size_t totalNumberOfElements;
  bool metaDataBuilt;
  int numDataRanks;
  int numVars;                                  // number of variables in the data (vx, vy, ...)
  std::vector<GIOPvPlugin::GioData> readInData; // the data readin

  std::vector<vtkDataArray*> tupleArray;
  std::vector<ParaviewField> paraviewData; // data paraview shows

  vtkIdType idx;
  int totalPoints;

  // Random numbers
  std::vector<size_t> _num;
  bool randomNumGenerated;
  size_t nextHash;

  // data
  std::string dataFilename;
  std::string currentFilename;

  // timeseries
  bool justLoaded;

  // Log
  GIOPvPlugin::Log debugLog;
  std::stringstream msgLog;
};

#endif
