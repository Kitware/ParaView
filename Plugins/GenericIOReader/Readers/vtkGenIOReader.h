// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright (c) 2017, Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov

#ifndef vtkGenIOReader_h
#define vtkGenIOReader_h

#include "vtkCellArray.h"             // for vtkCellArray
#include "vtkDataArraySelection.h"    // for vtkDataArraySelection
#include "vtkGenericIOReaderModule.h" // for export macro
#include "vtkPoints.h"                // for vtkPoints
#include "vtkSmartPointer.h"          // for vtkSmartPointer
#include "vtkUnstructuredGridAlgorithm.h"

#include "utils/gioData.h" // genio header
#include "utils/log.h"     // genio header

#include <mutex>   // for std::mutex
#include <sstream> // for std::stringstream
#include <string>  // for std::string
#include <vector>  // for std::vector

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

  vtkGenIOReader(const vtkGenIOReader&) = delete;
  void operator=(const vtkGenIOReader&) = delete;

protected:
  vtkGenIOReader();
  ~vtkGenIOReader() override;

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
