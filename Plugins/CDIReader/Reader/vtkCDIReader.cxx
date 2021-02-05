// -*- c++ -*-
/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    vtkCDIReader.cxx
 *
 *  Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
 *  All rights reserved.
 *  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 *
 *     This software is distributed WITHOUT ANY WARRANTY; without even
 *     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *     PURPOSE.  See the above copyright notice for more information.
 *
 *  =========================================================================*/
// .NAME vtkCDIReader - reads ICON/CDI netCDF data sets
// .SECTION Description
// vtkCDIReader is based on the vtk MPAS netCDF reader developed by
// Christine Ahrens (cahrens@lanl.gov). The plugin reads all ICON/CDI
// netCDF data sets with Point and cell variables, both 2D and 3D. It allows
// spherical (standard), as well as equidistant cylindrical and Cassini projection.
// 3D data can be visualized using slices, as well as 3D unstructured mesh. If
// bathymetry information (wet_c) is present in the data, this can be used for
// masking out continents. For more information, also check out our ParaView tutorial:
// https://www.dkrz.de/Nutzerportal-en/doku/vis/sw/paraview
//
// .SECTION Caveats
// The integrated visualization of performance data is not yet fully developed
// and documented. If interested in using it, see the following presentation
// https://www.dkrz.de/about/media/galerie/Vis/performance/perf-vis
// and/or contact Niklas Roeber at roeber@dkrz.de
//
// .SECTION Thanks
// Thanks to Uwe Schulzweida for the CDI code (uwe.schulzweida@mpimet.mpg.de)
// Thanks to Moritz Hanke for the sorting code (hanke@dkrz.de)

#include "vtkCDIReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFileSeriesReader.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/FStream.hxx"

// #include "timebase.h"

#include "vtk_netcdf.h"

#include "cdi_tools.h"
#include "helpers.h"
#include "projections.h"

#ifdef QT_CORE_LIB
#include <QDir>
#include <QFileDialog>
#endif

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
using namespace std;

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

struct Point
{
  double lon;
  double lat;
};

struct PointWithIndex
{
  Point p;
  int i;
};

//----------------------------------------------------------------------------
// Internal class to avoid name pollution
//----------------------------------------------------------------------------
class vtkCDIReader::Internal
{
public:
  Internal()
  {
    for (int i = 0; i < MAX_VARS; i++)
    {
      this->CellVarIDs[i] = -1;
      this->DomainVars[i] = std::string("");
    }
  }
  ~Internal() = default;

  int CellVarIDs[MAX_VARS];
  CDIVar CellVars[MAX_VARS];
  CDIVar PointVars[MAX_VARS];
  string DomainVars[MAX_VARS];

  // The Point data we expect to receive from each process.
  vtkSmartPointer<vtkIdTypeArray> PointsExpectedFromProcessesLengths;
  vtkSmartPointer<vtkIdTypeArray> PointsExpectedFromProcessesOffsets;

  // The Point data we have to send to each process.  Stored as global ids.
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcesses;
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcessesLengths;
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcessesOffsets;
};

namespace
{

//----------------------------------------------------------------------------
// Macro to check new didn't return an error
//----------------------------------------------------------------------------
#define CHECK_NEW(ptr)                                                                             \
  if ((ptr) == nullptr)                                                                            \
  {                                                                                                \
    vtkErrorMacro("new failed!" << endl);                                                          \
    return 0;                                                                                      \
  }

//----------------------------------------------------------------------------
//  Macros for template calls to load cell/Point variables
//----------------------------------------------------------------------------
#define vtkICONTemplateMacro(call)                                                                 \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call);                                                  \
  vtkTemplateMacroCase(VTK_FLOAT, float, call);

#define vtkICONTemplateDispatch(type, call)                                                        \
  switch (type)                                                                                    \
  {                                                                                                \
    vtkICONTemplateMacro(call);                                                                    \
    default:                                                                                       \
      vtkErrorMacro("Unsupported data type: " << (type));                                          \
      abort();                                                                                     \
  }

//----------------------------------------------------------------------------
// Routines for sorting and efficient removal of duplicates
// (c) and thanks to Moritz Hanke (DKRZ)
//----------------------------------------------------------------------------
int ComparePointWithIndex(const void* a, const void* b)
{
  const struct PointWithIndex* a_ = (struct PointWithIndex*)a;
  const struct PointWithIndex* b_ = (struct PointWithIndex*)b;
  double threshold = 1e-22;

  int lon_diff = fabs(a_->p.lon - b_->p.lon) > threshold;
  if (lon_diff)
  {
    return (a_->p.lon > b_->p.lon) ? -1 : 1;
  }

  int lat_diff = fabs(a_->p.lat - b_->p.lat) > threshold;
  if (lat_diff)
  {
    return (a_->p.lat > b_->p.lat) ? -1 : 1;
  }
  return 0;
}
}

vtkStandardNewMacro(vtkCDIReader);
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"
vtkCxxSetObjectMacro(vtkCDIReader, Controller, vtkMultiProcessController);
#endif

//----------------------------------------------------------------------------
// Constructor for vtkCDIReader
//----------------------------------------------------------------------------
vtkCDIReader::vtkCDIReader()
{
  // Debugging
  if (DEBUG)
  {
    this->DebugOn();
  }
  vtkDebugMacro("Starting to create vtkCDIReader..." << endl);
  this->initialized = false;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internals = new vtkCDIReader::Internal;
  this->StreamID = -1;
  this->VListID = -1;
  this->LoadingDimensions = vtkSmartPointer<vtkIntArray>::New();
  this->VariableDimensions = vtkStringArray::New();
  this->AllDimensions = vtkStringArray::New();
  this->AllVariableArrayNames = vtkSmartPointer<vtkStringArray>::New();
  this->InfoRequested = false;
  this->DataRequested = false;
  this->HaveDomainData = false;

  // Setup selection callback to modify this object when array selection changes
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->DomainDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkCDIReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->DomainDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (!this->Controller)
  {
    this->SetController(vtkDummyController::New());
  }
#endif

  this->Output = vtkSmartPointer<vtkUnstructuredGrid>::New();

  this->SetDefaults();

  vtkDebugMacro("MAX_VARS:" << MAX_VARS << endl);
  vtkDebugMacro("Created vtkCDIReader" << endl);
}

//----------------------------------------------------------------------------
//  Destroys data stored for variables, points, and cells, but
//  doesn't destroy the list of variables or toplevel cell/pointVarDataArray.
//----------------------------------------------------------------------------
void vtkCDIReader::DestroyData()
{
  vtkDebugMacro("DestroyData..." << endl);
  vtkDebugMacro("Destructing double cell var data..." << endl);

  // this->Internals->CellVars.clear();
  // this->Internals->PointVars.clear();
  /*
    delete [] this->MaximumLevelPoint;
    this->MaximumLevelPoint = nullptr;
    */

  if (this->CellVarDataArray)
  {
    for (int i = 0; i < this->NumberOfCellVars; i++)
    {
      if (this->CellVarDataArray[i] != nullptr)
      {
        this->CellVarDataArray[i]->Delete();
        this->CellVarDataArray[i] = nullptr;
      }
    }
  }

  vtkDebugMacro("Destructing double Point var array..." << endl);
  if (this->PointVarDataArray)
  {
    for (int i = 0; i < this->NumberOfPointVars; i++)
    {
      if (this->PointVarDataArray[i] != nullptr)
      {
        this->PointVarDataArray[i]->Delete();
        this->PointVarDataArray[i] = nullptr;
      }
    }
  }

  if (this->DomainVarDataArray)
  {
    for (int i = 0; i < this->NumberOfDomainVars; i++)
    {
      if (this->DomainVarDataArray[i] != nullptr)
      {
        this->DomainVarDataArray[i]->Delete();
        this->DomainVarDataArray[i] = nullptr;
      }
    }
  }

  if (this->ReconstructNew)
  {
    delete[] this->PointVarData;
    this->PointVarData = nullptr;
  }

  vtkDebugMacro("Out DestroyData..." << endl);
}

//----------------------------------------------------------------------------
// Destructor for vtkCDIReader
//----------------------------------------------------------------------------
vtkCDIReader::~vtkCDIReader()
{
  vtkDebugMacro("Destructing vtkCDIReader..." << endl);
  this->SetFileName(nullptr);

  if (this->StreamID >= 0)
  {
    streamClose(this->StreamID);
    this->StreamID = -1;
  }

  this->DestroyData();

  delete[] this->CellVarDataArray;
  this->CellVarDataArray = nullptr;

  delete[] this->PointVarDataArray;
  this->PointVarDataArray = nullptr;

  delete[] this->DomainVarDataArray;
  this->DomainVarDataArray = nullptr;

  delete[] this->DomainMask;
  this->DomainMask = nullptr;

  vtkDebugMacro("Destructing other stuff..." << endl);
  if (this->PointDataArraySelection)
  {
    this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
    this->PointDataArraySelection->Delete();
    this->PointDataArraySelection = nullptr;
  }
  if (this->CellDataArraySelection)
  {
    this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
    this->CellDataArraySelection->Delete();
    this->CellDataArraySelection = nullptr;
  }
  if (this->DomainDataArraySelection)
  {
    this->DomainDataArraySelection->RemoveObserver(this->SelectionObserver);
    this->DomainDataArraySelection->Delete();
    this->DomainDataArraySelection = nullptr;
  }
  if (this->SelectionObserver)
  {
    this->SelectionObserver->Delete();
    this->SelectionObserver = nullptr;
  }

  delete this->Internals;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  this->SetController(nullptr);
#endif

  this->VariableDimensions->Delete();
  this->AllDimensions->Delete();

  vtkDebugMacro("Destructed vtkCDIReader" << endl);
}

// Grab info from outInfo object (method extracted from RequestInformation)
void vtkCDIReader::GetFileSeriesInformation(vtkInformation* outInfo)
{
  if (outInfo->Has(vtkFileSeriesReader::FILE_SERIES_NUMBER_OF_FILES()))
  {
    this->NumberOfFiles = outInfo->Get(vtkFileSeriesReader::FILE_SERIES_NUMBER_OF_FILES());
  }
  if (outInfo->Has(vtkFileSeriesReader::FILE_SERIES_CURRENT_FILE_NUMBER()))
  {
    this->FileSeriesNumber = outInfo->Get(vtkFileSeriesReader::FILE_SERIES_CURRENT_FILE_NUMBER());
  }
  if (outInfo->Has(vtkFileSeriesReader::FILE_SERIES_FIRST_FILENAME()))
  {
    this->FileSeriesFirstName = outInfo->Get(vtkFileSeriesReader::FILE_SERIES_FIRST_FILENAME());
  }
}

//----------------------------------------------------------------------------
// Verify that the file exists, get dimension sizes and variables
//----------------------------------------------------------------------------
int vtkCDIReader::RequestInformation(
  vtkInformation* reqInfo, vtkInformationVector** inVector, vtkInformationVector* outVector)
{
  vtkDebugMacro("In vtkCDIReader::RequestInformation" << endl);
  if (!this->Superclass::RequestInformation(reqInfo, inVector, outVector))
  {
    return 0;
  }

  if (this->FileName.empty())
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    this->Decomposition = true;
    this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();
  }
#endif

  vtkDebugMacro(
    "In vtkCDIReader::RequestInformation read filename ok: " << this->FileName.c_str() << endl);
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  vtkDebugMacro("FileName: " << this->FileName.c_str() << endl);

  if (!this->GetDims())
  {
    return 0;
  }

  this->InfoRequested = true;

  vtkDebugMacro("In vtkCDIReader::RequestInformation setting VerticalLevelRange" << endl);
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = this->MaximumNVertLevels - 1;

  if (!this->BuildVarArrays())
  {
    return 0;
  }

  ResetVarDataArrays();

  GetFileSeriesInformation(outInfo);

  vtkSmartPointer<vtkDoubleArray> timeValues = ReadTimeAxis();

  OutputTimeAxis(outInfo, timeValues);

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  vtkDebugMacro("Out vtkCDIReader::RequestInformation" << endl);

  return 1;
}

vtkSmartPointer<vtkDoubleArray> vtkCDIReader::ReadTimeAxis()
{
  if ((this->FileSeriesNumber == 0) && (!this->TimeSet))
  {

    int taxisID = vlistInqTaxis(this->VListID);
    int calendar = taxisInqCalendar(taxisID);
    streamInqTimestep(this->StreamID, 0);
    int vdate = taxisInqVdate(taxisID);
    int vtime = taxisInqVtime(taxisID);

    this->FirstDay = date_to_julday(calendar, vdate);
    this->TimeUnits = ParseTimeUnits(vdate, vtime);
    this->Calendar = ParseCalendar(calendar);
    this->TimeSet = true;
  }

  if (!this->TimeSeriesTimeStepsAllSet)
  {
    if (this->TimeSeriesTimeSteps.size() < this->FileSeriesNumber + 1)
      this->TimeSeriesTimeSteps.resize(this->FileSeriesNumber + 1);
    this->TimeSeriesTimeSteps[this->FileSeriesNumber] = this->NumberOfTimeSteps;
  }

  this->NumberOfAllTimeSteps = 0;
  for (int step = 0; step < this->NumberOfFiles; step++)
  {
    this->NumberOfAllTimeSteps += this->TimeSeriesTimeSteps[step];
  }

  VTK_CREATE(vtkDoubleArray, timeValues);
  timeValues->Allocate(this->NumberOfTimeSteps);
  timeValues->SetNumberOfComponents(1);
  int start = 0;
  int counter = 0;
  for (int step = 0; step < this->FileSeriesNumber; step++)
  {
    start += this->TimeSeriesTimeSteps[step];
  }
  int end = start + this->NumberOfTimeSteps;
  for (int step = start; step < end; step++)
  {
    int taxisID = vlistInqTaxis(this->VListID);
    int calendar = taxisInqCalendar(taxisID);
    streamInqTimestep(this->StreamID, counter);
    int vdate = taxisInqVdate(taxisID);
    int vtime = taxisInqVtime(taxisID);
    double timevalue = date_to_julday(calendar, vdate);
    if (vtime > 0)
    {
      timevalue += (double)((time_to_sec(vtime)) / 86400.0);
    }
    timevalue -= this->FirstDay;
    if (timevalue >= 0.0)
    {
      timeValues->InsertNextTuple1(timevalue);
      if (!this->TimeSeriesTimeStepsAllSet)
      {
        if (TimeSteps.size() < step + 1)
          TimeSteps.resize(step + 1);
        this->TimeSteps[step] = timevalue;
      }
    }
    else
    {
      this->NumberOfTimeSteps -= 1;
    }
    counter++;
  }

  if (this->FileSeriesNumber == (this->NumberOfFiles - 1))
  {
    this->TimeSeriesTimeStepsAllSet = true;
  }

  return timeValues;
}

// Set the VarDataArrays to a bunch of null pointers.
// Call DestroyData() before calling this!
void vtkCDIReader::ResetVarDataArrays()
{
  delete[] this->PointVarDataArray;
  this->PointVarDataArray = new vtkDataArray*[this->NumberOfPointVars];
  for (int i = 0; i < this->NumberOfPointVars; i++)
  {
    this->PointVarDataArray[i] = nullptr;
  }

  delete[] this->CellVarDataArray;
  this->CellVarDataArray = new vtkDataArray*[this->NumberOfCellVars];
  for (int i = 0; i < this->NumberOfCellVars; i++)
  {
    this->CellVarDataArray[i] = nullptr;
  }

  delete[] this->DomainVarDataArray;
  this->DomainVarDataArray = new vtkDoubleArray*[this->NumberOfDomainVars];
  for (int i = 0; i < this->NumberOfDomainVars; i++)
  {
    this->DomainVarDataArray[i] = nullptr;
  }
}

// Add the time axis to the out info
void vtkCDIReader::OutputTimeAxis(
  vtkInformation* outInfo, const vtkSmartPointer<vtkDoubleArray>& timeValues)
{
  if (this->NumberOfTimeSteps > 0)
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeValues->GetPointer(0),
      timeValues->GetNumberOfTuples());
    double timeRange[2];
    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(timeValues->GetNumberOfTuples() - 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}

//----------------------------------------------------------------------------
int vtkCDIReader::getTimeIndex(double DataTimeStep)
{
  int start = 0;
  for (int step = 0; step < this->FileSeriesNumber; step++)
    start += this->TimeSeriesTimeSteps[step];
  int end = start + this->TimeSeriesTimeSteps[this->FileSeriesNumber];
  for (int step = start; step < end; step++)
  {
    if (this->TimeSteps[step] == DataTimeStep)
      return (step - start);
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkCDIReader::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  // make sure piece is valid
  return (piece < 0 || piece >= numPieces) ? 0 : 1;
}

//----------------------------------------------------------------------------
long vtkCDIReader::GetPartitioning(int piece, int numPieces, int numCellsPerLevel,
  int numPointsPerCell, int& beginPoint, int& endPoint, int& beginCell, int& endCell)
{
  if (numPieces == 1)
  {
    beginPoint = 0;
    endPoint = (numCellsPerLevel * numPointsPerCell) - 1;
    beginCell = 0;
    endCell = numCellsPerLevel - 1;

    return numCellsPerLevel;
  }
  else
  {
    long localCells = 0;
    int cells_per_piece = floor(numCellsPerLevel / numPieces);
    if (piece == 0)
    {
      beginCell = 0;
      endCell = (piece + 1) * cells_per_piece - 1;
      beginPoint = 0;
      endPoint = ((endCell + 1) * numPointsPerCell) - 1;
      localCells = 1 + endCell;
    }
    else if (piece < (numPieces - 1))
    {
      beginCell = piece * cells_per_piece;
      endCell = (piece + 1) * cells_per_piece;
      beginPoint = beginCell * numPointsPerCell;
      endPoint = (endCell * numPointsPerCell) - 1;
      localCells = endCell - beginCell;
    }
    else if (piece == (numPieces - 1))
    {
      beginCell = piece * cells_per_piece;
      endCell = numCellsPerLevel - 1;
      beginPoint = beginCell * numPointsPerCell;
      endPoint = ((endCell + 1) * numPointsPerCell) - 1;
      localCells = 1 + endCell - beginCell;
    }
    return localCells;
  }
}

//----------------------------------------------------------------------------
// Default method: Data is read into a vtkUnstructuredGrid
//----------------------------------------------------------------------------
int vtkCDIReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  vtkDebugMacro("In vtkCDIReader::RequestData" << endl);
  //  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outVector);
  //  this->Output = output;
  vtkUnstructuredGrid* output = this->Output;
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  if (outInfo->Has(vtkFileSeriesReader::FILE_SERIES_CURRENT_FILE_NUMBER()))
  {
    this->FileSeriesNumber = outInfo->Get(vtkFileSeriesReader::FILE_SERIES_CURRENT_FILE_NUMBER());
  }

  this->Piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->NumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  this->NumberLocalCells = this->GetPartitioning(this->Piece, this->NumPieces, this->NumberOfCells,
    this->PointsPerCell, this->BeginPoint, this->EndPoint, this->BeginCell, this->EndCell);

  if (this->DataRequested)
  {
    this->DestroyData();
  }
  if ((!initialized) || (!SkipGrid))
  {
    if (!this->ReadAndOutputGrid(true))
    {
      return 0;
    }
  }
  initialized = true;

  double requestedTimeStep = 0.;
#ifndef NDEBUG
  int numRequestedTimeSteps = 0;
#endif

  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  if (outInfo->Has(timeKey))
  {
#ifndef NDEBUG
    numRequestedTimeSteps = 1;
#endif
    requestedTimeStep = outInfo->Get(timeKey);
  }

  vtkDebugMacro("Num Time steps requested: " << numRequestedTimeSteps << endl);
  this->DTime = requestedTimeStep;
  vtkDebugMacro("this->DTime: " << this->DTime << endl);
  double dTimeTemp = this->DTime;
  Output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTimeTemp);
  vtkDebugMacro("dTimeTemp: " << dTimeTemp << endl);
  this->DTime = dTimeTemp;

  for (int var = 0; var < this->NumberOfCellVars; var++)
  {
    if (this->GetCellArrayStatus(this->Internals->CellVars[var].Name))
    {
      vtkDebugMacro("Loading Cell Variable: " << this->Internals->CellVars[var].Name << endl);
      this->LoadCellVarData(var, this->DTime);
      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
    }
    else
      vtkDebugMacro("Ignoring Cell Variable: " << this->Internals->CellVars[var].Name
                                               << " as requested " << endl);
  }
  for (int var = 0; var < this->NumberOfPointVars; var++)
  {
    if (this->GetPointArrayStatus(this->Internals->PointVars[var].Name))
    {
      vtkDebugMacro("Loading Point Variable: " << var << endl);
      this->LoadPointVarData(var, this->DTime);
      Output->GetPointData()->AddArray(this->PointVarDataArray[var]);
    }
  }

  for (int var = 0; var < this->NumberOfDomainVars; var++)
  {
    if (this->GetDomainArrayStatus(this->Internals->DomainVars[var].c_str()))
    {
      vtkDebugMacro(
        "Loading Domain Variable: " << this->Internals->DomainVars[var].c_str() << endl);
      this->LoadDomainVarData(var);
      Output->GetFieldData()->AddArray(this->DomainVarDataArray[var]);
    }
  }

  if (this->TimeUnits != "")
  {
    vtkNew<vtkStringArray> arr;
    arr->SetName("time_units");
    arr->InsertNextValue(this->TimeUnits);
    Output->GetFieldData()->AddArray(arr);
  }
  if (this->Calendar != "")
  {
    vtkNew<vtkStringArray> arr;
    arr->SetName("time_calendar");
    arr->InsertNextValue(this->Calendar);
    Output->GetFieldData()->AddArray(arr);
  }

  if (this->BuildDomainArrays)
  {
    this->BuildDomainArrays = this->BuildDomainCellVars();
  }

  this->DataRequested = true;
  vtkDebugMacro("Returning from RequestData" << endl);

  vtkUnstructuredGrid::GetData(outVector)->ShallowCopy(Output);

  return 1;
}

//----------------------------------------------------------------------------
// Regenerate and reread the data variables available
//----------------------------------------------------------------------------
int vtkCDIReader::RegenerateVariables()
{
  vtkDebugMacro("In RegenerateVariables" << endl);
  this->NumberOfPointVars = 0;
  this->NumberOfCellVars = 0;
  this->NumberOfDomainVars = 0;

  if (!this->GetDims())
  {
    return 0;
  }

  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = this->MaximumNVertLevels - 1;

  if (!this->BuildVarArrays())
  {
    return 0;
  }

  // Allocate the ParaView data arrays which will hold the variables
  this->ResetVarDataArrays();

  vtkDebugMacro("Returning from RegenerateVariables" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// If the user changes the projection, or singlelayer to
// multilayer, we need to regenerate the geometry.
//----------------------------------------------------------------------------
int vtkCDIReader::RegenerateGeometry()
{
  vtkUnstructuredGrid* output = this->Output;
  vtkDebugMacro("RegenerateGeometry ..." << endl);

  if (this->GridReconstructed)
  {
    vtkDebugMacro("GridReconstructed!" << endl);
    if (!this->ReadAndOutputGrid(true))
    {
      return 0;
    }
  }
  else
  {
    vtkDebugMacro("GridReconstructed=false" << endl);
  }

  double dTimeTemp = this->DTime;
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTimeTemp);
  this->DTime = dTimeTemp;

  for (int var = 0; var < this->NumberOfCellVars; var++)
  {
    if (this->GetCellArrayStatus(this->Internals->CellVars[var].Name))
    {
      vtkDebugMacro("Loading Cell Variable: " << this->Internals->CellVars[var].Name << endl);
      this->LoadCellVarData(var, this->DTime);
      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
    }
  }
  for (int var = 0; var < this->NumberOfPointVars; var++)
  {
    if (this->GetPointArrayStatus(this->Internals->PointVars[var].Name))
    {
      vtkDebugMacro("Loading Point Variable: " << var << endl);
      this->LoadPointVarData(var, this->DTime);
      output->GetPointData()->AddArray(this->PointVarDataArray[var]);
    }
  }

  for (int var = 0; var < this->NumberOfDomainVars; var++)
  {
    if (this->GetDomainArrayStatus(this->Internals->DomainVars[var].c_str()))
    {
      vtkDebugMacro(
        "Loading Domain Variable: " << this->Internals->DomainVars[var].c_str() << endl);
      this->LoadDomainVarData(var);
      output->GetFieldData()->AddArray(this->DomainVarDataArray[var]);
    }
  }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
  this->Modified();

  return 1;
}

//----------------------------------------------------------------------------
// Set defaults for various parameters and initialize some variables
//----------------------------------------------------------------------------
void vtkCDIReader::SetDefaults()
{
  this->Grib = false;

  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = 1;
  this->VerticalLevelSelected = 0;
  this->LayerThicknessRange[0] = 0;
  this->LayerThicknessRange[1] = 100;
  this->LayerThickness = 50;
  this->bloat = 2.0;

  this->Layer0OffsetRange[0] = -50;
  this->Layer0OffsetRange[1] = 51;
  this->Layer0Offset = 1e-30;

  // this is hard coded for now but will change when data generation gets more mature
  this->PerformanceDataFile = "timer.atmo.";
  this->DomainVarName = "cell_owner";
  this->DomainDimension = "domains";
  this->HaveDomainVariable = false;
  this->HaveDomainData = false;

  this->DimensionSelection = 0;
  this->InvertZAxis = false;
  this->DoublePrecision = false;
  this->ShowClonClat = false;
  this->ProjectionMode = projection::SPHERICAL;
  this->ShowMultilayerView = false;
  this->ReconstructNew = false;
  this->CellDataSelected = 0;
  this->PointDataSelected = 0;
  this->MaskingVarname = "";
  this->GotMask = false;
  this->AddCoordinateVars = false;
  this->FilenameSet = false;
  this->NumberOfTimeSteps = 0;
  this->NumberOfAllTimeSteps = 0;
  this->TimeSeriesTimeSteps.reserve(5);
  this->TimeSteps.reserve(100 * 250 + 25);
  this->TimeSeriesTimeStepsAllSet = false;
  this->GridReconstructed = false;
  this->CustomMaskValue = 0.0;
  this->InvertMask = false;
  this->UseMask = false;
  this->UseCustomMaskValue = false;
  this->Decomposition = false;
  this->gridSelected = false;

  this->SkipGrid = false;

  this->BeginCell = 0;
  this->FirstDay = -1;
  this->TimeSet = false;

  this->DTime = 0;
  this->CellVarDataArray = nullptr;
  this->PointVarDataArray = nullptr;
  this->DomainVarDataArray = nullptr;
  this->PointVarData = nullptr;
  this->FileSeriesNumber = 0;
  this->NumberOfFiles = 1;
  this->NeedHorizontalGridFile = false;
  this->NeedVerticalGridFile = false;

  this->TStepDistance = 1.0;
  this->NumberOfProcesses = 1;

  this->BuildDomainArrays = false;
  this->MaximumNVertLevels = 0;
  this->MaximumPoints = 0;
  this->MaximumCells = 0;
  this->DepthVar = nullptr;

  this->DomainMask = new int[MAX_VARS];
  for (int i = 0; i < MAX_VARS; i++)
  {
    this->DomainMask[i] = 0;
  }

  this->NumberOfPoints = 0;
}

//----------------------------------------------------------------------------
// Get dimensions of key NetCDF variables
//----------------------------------------------------------------------------
int vtkCDIReader::OpenFile()
{
  // check if we got either *.Grib or *.nc data
  string file = this->FileName;
  string check = file.substr((file.size() - 4), file.size());
  if (check == "grib" || check == ".grb")
  {
    this->Grib = true;
  }
  else
  {
    this->Grib = false;
  }

  if (this->StreamID >= 0)
  {
    streamClose(this->StreamID);
    this->StreamID = -1;
    this->VListID = -1;
  }

  this->StreamID = streamOpenRead(this->FileNameGrid.c_str());
  if (this->StreamID < 0)
  {
    return 0;
  }

  vtkDebugMacro("In vtkCDIReader::RequestInformation read file okay" << endl);
  this->VListID = streamInqVlist(this->StreamID);

  int nvars = vlistNvars(this->VListID);
  char varname[CDI_MAX_NAME];
  for (int VarID = 0; VarID < nvars; ++VarID)
  {
    vlistInqVarName(this->VListID, VarID, varname);
  }

  return 1;
}

void vtkCDIReader::GuessGridFile()
{
  string fallback = ::GetPathName(this->FileName) + "/grid.nc";

  string guess = "";
  if (!this->Grib)
    guess = GuessGridFileFromUri(this->FileName);

  if (guess != "")
  {
    if (CheckFileAccess(guess))
    {
      this->FileNameGrid = guess;
      return;
    }
    else
      vtkWarningMacro("CDI Reader: Could not find grid file "
        << guess << " indicated by grid_file_uri attribute in " << this->FileName
        << " Trying fallback guess " << fallback << endl);
  }
  this->FileNameGrid = fallback;
}

//----------------------------------------------------------------------------
// Get dimensions of key NetCDF variables
//----------------------------------------------------------------------------
int vtkCDIReader::GetDims()
{
  if (!this->FileName.empty())
  {
    this->FileNameGrid = this->FileName;
    if (this->VListID < 0 || this->StreamID < 0)
    {
      if (!this->OpenFile())
      {
        return 0;
      }
    }

    this->ReadHorizontalGridData();
    if (this->NeedHorizontalGridFile)
    {
      // if there is no grid information in the data file, try opening
      // an additional grid file named grid.nc in the same directory to
      // read in the grid information
      if (this->StreamID >= 0)
      {
        streamClose(this->StreamID);
        this->StreamID = -1;
        this->VListID = -1;
      }

      char* directory = new char[strlen(this->FileName.c_str()) + 1];
      strcpy(directory, this->FileName.c_str());

      if (this->gridSelected == true)
      {
        this->FileNameGrid = this->FileNameGridSelect;
      }
      else
      {
        GuessGridFile();
      }
      if (!this->OpenFile())
      {
        return 0;
      }
      if (!this->ReadHorizontalGridData())
      {
        vtkErrorMacro("Couldn't open grid information in data nor in the grid file." << endl);
        return 0;
      }

      this->FileNameGrid = this->FileName;
      if (!this->OpenFile())
      {
        return 0;
      }
    }

    this->ReadVerticalGridData();
    if (this->NeedVerticalGridFile)
    {
      // if there is no grid information in the data file, try opening
      // an additional grid file named grid.nc in the same directory to
      // read in the grid information
      if (this->StreamID >= 0)
      {
        streamClose(this->StreamID);
        this->StreamID = -1;
        this->VListID = -1;
      }

      char* directory = new char[strlen(this->FileName.c_str()) + 1];
      strcpy(directory, this->FileName.c_str());
      if (!this->OpenFile())
      {
        return 0;
      }

      if (!this->ReadVerticalGridData())
      {
        vtkDebugMacro("Couldn't neither open grid information within the data netCDF file, nor "
                      "in the grid.nc file."
          << endl);
        vtkErrorMacro("Couldn't neither open grid information within the data netCDF file, nor "
                      "in the grid.nc file."
          << endl);
        return 0;
      }

      this->FileNameGrid = this->FileName;
      if (!this->OpenFile())
      {
        return 0;
      }
    }

    if (this->DimensionSelection > 0)
    {
      int ngrids = vlistNgrids(this->VListID);
      int nzaxis = vlistNzaxis(this->VListID);

      this->GridID = vlistGrid(this->VListID, floor(this->DimensionSelection / nzaxis));
      this->ZAxisID = vlistZaxis(this->VListID,
        this->DimensionSelection - (nzaxis * floor(this->DimensionSelection / nzaxis)));
    }

    if (this->GridID != -1)
    {
      this->NumberOfCells = static_cast<int>(gridInqSize(this->GridID));

      if (this->NumberOfPoints and
        this->NumberOfPoints != static_cast<int>(gridInqSize(this->GridID)))
        vtkWarningMacro("CDI-Reader:GetDims: Changing number of points from  "
          << this->NumberOfPoints << " to " << static_cast<int>(gridInqSize(this->GridID)) << endl);
      this->NumberOfPoints = static_cast<int>(gridInqSize(this->GridID));
      this->PointsPerCell = gridInqNvertex(this->GridID);
    }

    int ntsteps = 0;
    if (this->Grib)
    {
      while (streamInqTimestep(this->StreamID, ntsteps))
        ntsteps++;
    }
    else
    {
      ntsteps = vlistNtsteps(this->VListID);
    }
    this->NumberOfTimeSteps = ntsteps;

    this->MaximumNVertLevels = 1;
    if (this->ZAxisID != -1)
    {
      this->MaximumNVertLevels = zaxisInqSize(this->ZAxisID);
    }

    this->FillGridDimensions();
  }
  else
  {
    vtkDebugMacro("No Filename yet set!" << endl);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Read Horizontal Grid Data
//----------------------------------------------------------------------------
int vtkCDIReader::ReadHorizontalGridData()
{
  int vlistID_l = this->VListID;
  this->GridID = -1;
  this->ZAxisID = -1;
  this->SurfID = -1;

  int ngrids = vlistNgrids(vlistID_l);
  for (int i = 0; i < ngrids; ++i)
  {
    int gridID_l = vlistGrid(vlistID_l, i);
    int nv = gridInqNvertex(gridID_l);

    if (nv >= 3) //  ((nv == 3 || nv == 4)) // && gridInqType(gridID_l) == GRID_UNSTRUCTURED)
    {
      this->GridID = gridID_l;
      break;
    }
  }

  if (this->GridID == -1)
  {
    this->NeedHorizontalGridFile = true;
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Read Vertical Grid Data
//----------------------------------------------------------------------------
int vtkCDIReader::ReadVerticalGridData()
{
  this->ZAxisID = -1;
  this->SurfID = -1;
  int nzaxis = vlistNzaxis(this->VListID);

  for (int i = 0; i < nzaxis; ++i)
  {
    int zaxisID_l = vlistZaxis(this->VListID, i);
    if (zaxisInqSize(zaxisID_l) == 1 || zaxisInqType(zaxisID_l) == ZAXIS_SURFACE)
    {
      this->SurfID = zaxisID_l;
      this->ZAxisID = zaxisID_l;
      break;
    }
  }

  for (int i = 0; i < nzaxis; ++i)
  {
    int zaxisID_l = vlistZaxis(this->VListID, i);
    if (zaxisInqSize(zaxisID_l) > 1)
    {
      this->ZAxisID = zaxisID_l;
      break;
    }
  }

  if (this->ZAxisID == -1)
  {
    this->NeedVerticalGridFile = true;
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Get the NetCDF variables on cell and vertex and check for domain data
//----------------------------------------------------------------------------
int vtkCDIReader::GetVars()
{
  int cellVarIndex = -1;
  int pointVarIndex = -1;
  int domainVarIndex = -1;

  int numVars = vlistNvars(this->VListID);
  for (int i = 0; i < numVars; i++)
  {
    int VarID = i;
    CDIVar aVar;

    aVar.StreamID = StreamID;
    aVar.VarID = VarID;
    aVar.GridID = vlistInqVarGrid(this->VListID, VarID);
    aVar.ZAxisID = vlistInqVarZaxis(this->VListID, VarID);
    aVar.GridSize = static_cast<int>(gridInqSize(aVar.GridID));
    aVar.NLevel = zaxisInqSize(aVar.ZAxisID);
    aVar.Type = 0;
    aVar.ConstTime = 0;

    // to do multiple grids:
    // - Check how many grids are available
    // - Check if all grids can be reconstructed, or if bnds are all zero
    // - Reform gui to load either Cell, Point or Edge data

    if (vlistInqVarTsteptype(this->VListID, VarID) == TIME_CONSTANT)
    {
      aVar.ConstTime = 1;
    }
    if (aVar.ZAxisID != this->ZAxisID && aVar.ZAxisID != this->SurfID)
    {
      continue;
    }
    // if (gridInqType(aVar.GridID) != GRID_UNSTRUCTURED)
    // {
    //   continue;
    // }

    vlistInqVarName(this->VListID, VarID, aVar.Name);
    aVar.Type = 2;
    if (aVar.NLevel > 1)
    {
      aVar.Type = 3;
    }
    int varType = aVar.Type;

    // check for dim 2 being cell or Point
    bool isCellData = false;
    bool isPointData = false;
    if (aVar.GridSize == this->NumberOfCells)
    {
      isCellData = true;
    }
    else if ((aVar.GridSize < this->NumberOfCells) && (this->PointsPerCell == 3))
    {
      if (this->NumberOfPoints and this->NumberOfPoints != aVar.GridSize)
      {
        vtkWarningMacro("Not adding "
          << aVar.Name << " as point var, as it's size " << aVar.GridSize
          << " does not correspond to our understanding of the correct size for 'the' point grid: "
          << this->NumberOfPoints << endl);
        break;
      }
      isPointData = true;
      this->NumberOfPoints = aVar.GridSize;
    }
    else
    {
      continue;
    }

    // 3D variables ...
    if (varType == 3)
    {
      if (isCellData)
      {
        cellVarIndex++;
        if (cellVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro("Exceeded number of cell vars." << endl);
          return 0;
        }
        this->Internals->CellVars[cellVarIndex] = aVar;
        vtkDebugMacro("Adding var " << aVar.Name << " to CellVars" << endl);
      }
      else if (isPointData)
      {
        pointVarIndex++;
        if (pointVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro("Exceeded number of Point vars." << endl);
          return 0;
        }
        this->Internals->PointVars[pointVarIndex] = aVar;
        vtkDebugMacro("Adding var " << aVar.Name << " to PointVars" << endl);
      }
    }
    // 2D variables
    else if (varType == 2)
    {
      vtkDebugMacro("check for " << aVar.Name << "." << endl);
      if (isCellData)
      {
        cellVarIndex++;
        if (cellVarIndex > MAX_VARS - 1)
        {
          vtkDebugMacro("Exceeded number of cell vars." << endl);
          return 0;
        }
        this->Internals->CellVars[cellVarIndex] = aVar;
        vtkDebugMacro("Adding var " << aVar.Name << " to CellVars" << endl);
      }
      else if (isPointData)
      {
        pointVarIndex++;
        if (pointVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro("Exceeded number of Point vars." << endl);
          return 0;
        }
        this->Internals->PointVars[pointVarIndex] = aVar;
        vtkDebugMacro("Adding var " << aVar.Name << " to PointVars" << endl);
      }
    }
  }

  // check if domain var is defined and how many domains are available
  for (int var = 0; var < pointVarIndex + 1; var++)
  {
    if (!strcmp(this->Internals->PointVars[var].Name, this->DomainVarName.c_str()))
    {
      this->HaveDomainVariable = true;
    }
  }
  for (int var = 0; var < cellVarIndex + 1; var++)
  {
    if (!strcmp(this->Internals->CellVars[var].Name, this->DomainVarName.c_str()))
    {
      this->HaveDomainVariable = true;
    }
  }

  // prepare data structure and read in names
  string filename = PerformanceDataFile + "0000";
  vtksys::ifstream file(filename.c_str());
  if (file.good())
  {
    this->HaveDomainData = true;
  }

  if (this->SupportDomainData())
  {
    vtkDebugMacro("Found Domain Data and loading it." << endl);
    this->BuildDomainArrays = true;
  }

  this->NumberOfPointVars = pointVarIndex + 1;
  this->NumberOfCellVars = cellVarIndex + 1;
  this->NumberOfDomainVars = domainVarIndex + 1;
  this->NumberAllCells = this->NumberOfCells;
  this->NumberAllPoints = this->NumberOfPoints;

  return 1;
}

//----------------------------------------------------------------------------
// Build the selection Arrays for points and cells in the GUI.
//----------------------------------------------------------------------------
int vtkCDIReader::BuildVarArrays()
{
  vtkDebugMacro("In vtkCDIReader::BuildVarArrays" << endl);

  if (!this->FileName.empty())
  {
    if (!GetVars())
    {
      return 0;
    }

    vtkDebugMacro("NumberOfCellVars: " << this->NumberOfCellVars << " NumberOfPointVars: "
                                       << this->NumberOfPointVars << endl);
    if (this->NumberOfCellVars == 0)
    {
      vtkErrorMacro("No cell variables found!" << endl);
    }

    for (int var = 0; var < this->NumberOfPointVars; var++)
    {
      this->PointDataArraySelection->EnableArray(this->Internals->PointVars[var].Name);
      vtkDebugMacro("Adding Point var: " << this->Internals->PointVars[var].Name << endl);
    }

    for (int var = 0; var < this->NumberOfCellVars; var++)
    {
      vtkDebugMacro("Adding cell var: " << this->Internals->CellVars[var].Name << endl);
      this->CellDataArraySelection->EnableArray(this->Internals->CellVars[var].Name);
    }

    for (int var = 0; var < this->NumberOfDomainVars; var++)
    {
      vtkDebugMacro("Adding domain var: " << this->Internals->DomainVars[var].c_str() << endl);
      this->DomainDataArraySelection->EnableArray(this->Internals->DomainVars[var].c_str());
    }
  }
  else
  {
    vtkDebugMacro("No Filename yet set." << endl);
  }

  vtkDebugMacro("Leaving vtkCDIReader::BuildVarArrays" << endl);
  return 1;
}

//----------------------------------------------------------------------------
//  Read the data from the ncfile, allocate the geometry and create the
//  vtk data structures for points and cells.
//----------------------------------------------------------------------------
int vtkCDIReader::ReadAndOutputGrid(bool init)
{
  vtkDebugMacro("In vtkCDIReader::ReadAndOutputGrid" << endl);

  if (this->ProjectionMode == projection::SPHERICAL)
  {
    if (!this->AllocSphereGeometry())
    {
      return 0;
    }
  }
  else
  {
    if (!this->AllocLatLonGeometry())
    {
      return 0;
    }

    if (this->ProjectionMode == projection::CASSINI)
    {
      if (!this->Wrap(2))
      {
        return 0;
      }
    }
    else
    {
      if (!this->Wrap(1))
      {
        return 0;
      }
    }
  }

  if ((this->ProjectionMode == projection::CYLINDRICAL_EQUIDISTANT) ||
    (this->ProjectionMode == projection::CASSINI))
  {
    this->AddMaskHalo();
    this->AddClonClatHalo();
  }
  this->OutputPoints(init);
  this->OutputCells(init);

  // Allocate the data arrays which will hold the NetCDF var data
  vtkDebugMacro("pointVarData: Alloc " << this->MaximumPoints << " doubles" << endl);
  delete[] this->PointVarData;
  this->PointVarData = new double[this->MaximumPoints];
  vtkDebugMacro("Leaving vtkCDIReader::ReadAndOutputGrid" << endl);

  return 1;
}

//----------------------------------------------------------------------------
// Mirrors the triangle mesh in z direction
//----------------------------------------------------------------------------
int vtkCDIReader::MirrorMesh()
{
  for (int i = 0; i < this->NumberLocalPoints; i++)
  {
    this->PointZ[i] = (this->PointZ[i] * (-1.0));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCDIReader::RemoveDuplicates(
  double* PointLon, double* PointLat, int temp_nbr_vertices, int* triangle_list, int* nbr_cells)
{
  struct PointWithIndex* sort_array = new PointWithIndex[temp_nbr_vertices];

  for (int i = 0; i < temp_nbr_vertices; ++i)
  {
    double curr_lon, curr_lat;
    double threshold = (vtkMath::Pi() / 2.0) - 1e-4;
    curr_lon = ((double*)PointLon)[i];
    curr_lat = ((double*)PointLat)[i];

    while (curr_lon < 0.0)
    {
      curr_lon += 2 * vtkMath::Pi();
    }
    while (curr_lon >= vtkMath::Pi())
    {
      curr_lon -= 2 * vtkMath::Pi();
    }

    if (curr_lat > threshold)
    {
      curr_lon = 0.0;
    }
    else if (curr_lat < (-1.0 * threshold))
    {
      curr_lon = 0.0;
    }

    sort_array[i].p.lon = curr_lon;
    sort_array[i].p.lat = curr_lat;
    sort_array[i].i = i;
  }

  qsort(sort_array, temp_nbr_vertices, sizeof(*sort_array), ::ComparePointWithIndex);
  triangle_list[sort_array[0].i] = 1;

  int last_unique_idx = sort_array[0].i;
  for (int i = 1; i < temp_nbr_vertices; ++i)
  {
    if (::ComparePointWithIndex(sort_array + i - 1, sort_array + i))
    {
      triangle_list[sort_array[i].i] = 1;
      last_unique_idx = sort_array[i].i;
    }
    else
    {
      triangle_list[sort_array[i].i] = -last_unique_idx;
    }
  }

  int new_nbr_vertices = 0;
  for (int i = 0; i < temp_nbr_vertices; ++i)
  {
    if (triangle_list[i] == 1)
    {
      PointLon[new_nbr_vertices] = PointLon[i];
      PointLat[new_nbr_vertices] = PointLat[i];
      triangle_list[i] = new_nbr_vertices;
      new_nbr_vertices++;
    }
  }

  for (int i = 0; i < temp_nbr_vertices; ++i)
  {
    if (triangle_list[i] <= 0)
    {
      triangle_list[i] = triangle_list[-triangle_list[i]];
    }
  }

  nbr_cells[0] = temp_nbr_vertices;
  nbr_cells[1] = new_nbr_vertices;
  delete[] sort_array;
}

//----------------------------------------------------------------------------
// Construct grid geometry
//----------------------------------------------------------------------------
int vtkCDIReader::ConstructGridGeometry()
{
  vtkDebugMacro("Starting grid reconstruction ..." << endl);
  if (this->NumberOfCellVars == 0 && this->NumberOfPointVars == 0)
  {
    vtkErrorMacro("No cell variables - can't construct grid" << endl);
    return 0;
  }

  this->NumberLocalCells = this->GetPartitioning(this->Piece, this->NumPieces, this->NumberOfCells,
    this->PointsPerCell, this->BeginPoint, this->EndPoint, this->BeginCell, this->EndCell);

  int size = this->NumberLocalCells * this->PointsPerCell;
  int size2 = this->NumberAllCells * this->PointsPerCell;
  this->CLonVertices = new double[size];
  this->CLatVertices = new double[size];
  this->DepthVar = new double[this->MaximumNVertLevels];
  CHECK_NEW(this->CLonVertices);
  CHECK_NEW(this->CLatVertices);
  CHECK_NEW(this->DepthVar);

  vtkDebugMacro("Start reading Vertices" << endl);
  gridInqXboundsPart(
    this->GridID, (this->BeginCell * this->PointsPerCell), size, this->CLonVertices);
  gridInqYboundsPart(
    this->GridID, (this->BeginCell * this->PointsPerCell), size, this->CLatVertices);
  vtkDebugMacro("Done reading Vertices" << endl);
  zaxisInqLevels(this->ZAxisID, this->DepthVar);
  char units[CDI_MAX_NAME];
  this->OrigConnections.resize(size);
  int new_cells[2];

  if (this->ProjectionMode != projection::CATALYST)
  {
    gridInqXunits(this->GridID, units);
    if (strncmp(units, "degree", 6) == 0)
    {
      for (int i = 0; i < size; i++)
      {
        this->CLonVertices[i] = vtkMath::RadiansFromDegrees(this->CLonVertices[i]);
      }
    }
    gridInqYunits(this->GridID, units);
    if (strncmp(units, "degree", 6) == 0)
    {
      for (int i = 0; i < size; i++)
      {
        this->CLatVertices[i] = vtkMath::RadiansFromDegrees(this->CLatVertices[i]);
      }
    }
  }

  // check for duplicates in the Point list and update the triangle list
  this->RemoveDuplicates(
    this->CLonVertices, this->CLatVertices, size, &this->OrigConnections[0], new_cells);
  this->NumberLocalCells = floor(new_cells[0] / this->PointsPerCell);
  this->NumberLocalPoints = new_cells[1];
  if (this->NumberOfPoints and this->NumberOfPoints != new_cells[1])
    vtkWarningMacro("CDI-Reader:ConstructGridGeometry: Changing number of points from  "
      << this->NumberOfPoints << " to " << new_cells[1] << endl);
  this->NumberOfPoints = new_cells[1];

  this->ModNumPoints = (int)floor(this->NumberLocalPoints * (this->bloat * this->bloat));
  this->ModNumCells = (int)floor(this->NumberLocalCells * (this->bloat));

  this->PointMap.resize((size_t)floor(this->NumberOfPoints * (this->bloat * this->bloat)));
  this->CellMap.resize((size_t)floor(this->NumberOfCells * this->bloat));

  this->PointX.resize(this->ModNumPoints);
  this->PointY.resize(this->ModNumPoints);
  this->PointZ.resize(this->ModNumPoints);

  // now get the individual coordinates out of the clon/clat vertices
  for (int i = 0; i < this->NumberLocalPoints; i++)
  {
    ::LLtoXYZ(this->CLonVertices[i], this->CLatVertices[i], &PointX[i], &PointY[i], &PointZ[i],
      this->ProjectionMode);
  }

  // mirror the mesh if needed
  if (ProjectionMode == SPHERICAL)
  {
    this->MirrorMesh();
  }
  this->GridReconstructed = true;
  this->ReconstructNew = false;

  // if we run with data decomposition, we need to know the mapping of points
  this->VertexIds.resize(size);
  int* vertex_ids2 = new int[size2];
  CHECK_NEW(vertex_ids2);
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (this->Decomposition)
  {
    if (this->Piece == 0)
    {
      int new_cells2[2];
      double clon_vert2[size2];
      double clat_vert2[size2];

      gridInqXboundsPart(this->GridID, 0, size2, clon_vert2);
      gridInqYboundsPart(this->GridID, 0, size2, clat_vert2);

      gridInqXunits(this->GridID, units);
      if (strncmp(units, "degree", 6) == 0)
      {
        for (int i = 0; i < size2; i++)
        {
          clon_vert2[i] = vtkMath::RadiansFromDegrees(clon_vert2[i]);
        }
      }

      gridInqYunits(this->GridID, units);
      if (strncmp(units, "degree", 6) == 0)
      {
        for (int i = 0; i < size2; i++)
        {
          clat_vert2[i] = vtkMath::RadiansFromDegrees(clat_vert2[i]);
        }
      }

      this->RemoveDuplicates(clon_vert2, clat_vert2, size2, vertex_ids2, new_cells2);
      for (int i = 1; i < this->NumPieces; i++)
      {
        this->Controller->Send(vertex_ids2, size2, i, 101);
      }
    }
    else
    {
      this->Controller->Receive(vertex_ids2, size2, 0, 101);

      for (int i = this->BeginPoint; i < (this->EndPoint + 1); i++)
      {
        this->VertexIds[i - this->BeginPoint] = vertex_ids2[i];
      }
    }

    this->SetupPointConnectivity();
  }
#endif

  this->CurrentExtraPoint = this->NumberLocalPoints;
  this->CurrentExtraCell = this->NumberLocalCells;

  delete[] this->CLonVertices;
  delete[] this->CLatVertices;
  this->CLonVertices = nullptr;
  this->CLatVertices = nullptr;
  delete[] vertex_ids2;
  vtkDebugMacro("Grid Reconstruction complete..." << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Allocate into sphere view of geometry
// This is work in progress, but as almost all variables are cell based, it
// is not that urgent.
//----------------------------------------------------------------------------
void vtkCDIReader::SetupPointConnectivity()
{
  this->Internals->PointsExpectedFromProcessesLengths = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internals->PointsExpectedFromProcessesLengths->SetNumberOfTuples(this->NumPieces);
  this->Internals->PointsExpectedFromProcessesOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internals->PointsExpectedFromProcessesOffsets->SetNumberOfTuples(this->NumPieces);
  this->Internals->PointsToSendToProcesses = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internals->PointsToSendToProcessesLengths = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internals->PointsToSendToProcessesLengths->SetNumberOfTuples(this->NumPieces);
  this->Internals->PointsToSendToProcessesOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internals->PointsToSendToProcessesOffsets->SetNumberOfTuples(this->NumPieces);
}

//----------------------------------------------------------------------------
// Allocate into sphere view of geometry
//----------------------------------------------------------------------------
int vtkCDIReader::AllocSphereGeometry()
{
  vtkDebugMacro("In AllocSphereGeometry..." << endl);

  if (!GridReconstructed || this->ReconstructNew)
  {
    this->ConstructGridGeometry();
  }

  if (this->ShowMultilayerView)
  {
    this->MaximumCells = this->NumberLocalCells * this->MaximumNVertLevels;
    this->MaximumPoints = this->NumberLocalPoints * (this->MaximumNVertLevels + 1);
  }
  else
  {
    this->MaximumCells = this->NumberLocalCells;
    this->MaximumPoints = this->NumberLocalPoints;
  }

  if (this->ShowClonClat)
  {
    this->LoadClonClatVars();
  }
  this->CheckForMaskData();
  vtkDebugMacro("Leaving AllocSphereGeometry...");
  return 1;
}

//----------------------------------------------------------------------------
// Allocate the lat/lon or Cassini projection of geometry.
//----------------------------------------------------------------------------
int vtkCDIReader::AllocLatLonGeometry()
{
  vtkDebugMacro("In AllocLatLonGeometry..." << endl);
  if (this->NumberOfCellVars == 0 && this->NumberOfPointVars == 0)
  {
    vtkErrorMacro("No cell or point variables - can't construct grid" << endl);
    return 0;
  }

  if (!GridReconstructed || this->ReconstructNew)
  {
    this->ConstructGridGeometry();
  }

  this->ModConnections.resize(this->ModNumCells * this->PointsPerCell);

  if (this->ShowMultilayerView)
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint * (this->MaximumNVertLevels + 1);
  }
  else
  {
    this->MaximumPoints = this->CurrentExtraPoint;
    this->MaximumCells = this->CurrentExtraCell;
  }

  if (this->ShowClonClat)
  {
    this->LoadClonClatVars();
  }
  this->CheckForMaskData();
  vtkDebugMacro("Leaving AllocLatLonGeometry..." << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Construct grid geometry
//----------------------------------------------------------------------------
int vtkCDIReader::LoadClonClatVars()
{
  vtkDebugMacro("In LoadClonClatVars..." << endl);

  double* CLon_l = new double[this->NumberLocalCells];
  double* CLat_l = new double[this->NumberLocalCells];
  CHECK_NEW(CLon_l);
  CHECK_NEW(CLat_l);

  gridInqXvalsPart(this->GridID, this->BeginCell, this->NumberLocalCells, CLon_l);
  gridInqYvalsPart(this->GridID, this->BeginCell, this->NumberLocalCells, CLat_l);

  char units[CDI_MAX_NAME];
  gridInqXunits(this->GridID, units);
  if (strncmp(units, "degree", 6) == 0)
  {
    for (int i = 0; i < this->NumberLocalCells; i++)
    {
      CLon_l[i] = vtkMath::RadiansFromDegrees(CLon_l[i]);
    }
  }
  gridInqYunits(this->GridID, units);
  if (strncmp(units, "degree", 6) == 0)
  {
    for (int i = 0; i < this->NumberLocalCells; i++)
    {
      CLat_l[i] = vtkMath::RadiansFromDegrees(CLat_l[i]);
    }
  }

  if (this->ShowMultilayerView)
  {
    int tmp = this->MaximumCells * this->bloat;
    this->CLon.resize(tmp);
    this->CLat.resize(tmp);

    for (int j = 0; j < this->NumberLocalCells; j++)
    {
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        int i = j * this->MaximumNVertLevels;
        this->CLon[i + levelNum] = static_cast<double>(CLon_l[j]);
        this->CLat[i + levelNum] = static_cast<double>(CLat_l[j]);
      }
    }
  }
  else
  {
    int tmp = this->NumberLocalCells * this->bloat;
    this->CLon.resize(tmp);
    this->CLat.resize(tmp);

    for (int j = 0; j < this->NumberLocalCells; j++)
    {
      this->CLon[j] = static_cast<double>(CLon_l[j]);
      this->CLat[j] = static_cast<double>(CLat_l[j]);
    }
  }

  delete[] CLon_l;
  delete[] CLat_l;

  this->AddCoordinateVars = true;
  vtkDebugMacro("Out LoadClonClatVars..." << endl);
  return 1;
}

int vtkCDIReader::AddClonClatHalo()
{
  if ((this->AddCoordinateVars) && (this->NumberLocalCells < this->CurrentExtraCell))
  {
    if (this->ShowMultilayerView)
    {
      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int l = j * this->MaximumNVertLevels;
          int k = this->MaximumNVertLevels * this->CellMap[j - this->NumberLocalCells];
          this->CLon[l + levelNum] = static_cast<int>(CLon[k + levelNum]);
          this->CLat[l + levelNum] = static_cast<int>(CLat[k + levelNum]);
        }
      }
    }
    else
    {
      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        int k = this->CellMap[j - this->NumberLocalCells];
        this->CLon[j] = static_cast<int>(CLon[k]);
        this->CLat[j] = static_cast<int>(CLat[k]);
      }
    }

    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
//  Read in Missing value mask. We probably should not read this, but just fetch it, but hell,
//  whatever.
//----------------------------------------------------------------------------
int vtkCDIReader::CheckForMaskData()
{
  int numVars = this->NumberOfCellVars;
  this->GotMask = false;
  int mask_pos = 0;

  string mask_name;
  if (this->PointsPerCell == 3)
    mask_name = "wet_c";
  if (this->PointsPerCell == 4)
    mask_name = "wet_e";
  if (this->PointsPerCell == 6)
    mask_name = "wet_v";

  for (int i = 0; i < numVars; i++)
  {
    if (!strcmp(this->Internals->CellVars[i].Name, this->MaskingVarname.c_str()))
    {
      if (this->MaskingVarname == "")
      {
        std::cerr << "Found empty var at position " << i << " of Internals->CellVars ";
        break;
      }

      this->GotMask = true;
      mask_pos = i;
      break;
    }
  }
  if (this->GotMask == false)
  {
    this->MaskingVarname = mask_name;
    for (int i = 0; i < numVars; i++)
    {
      if (!strcmp(this->Internals->CellVars[i].Name, MaskingVarname.c_str()))
      {
        if (this->MaskingVarname == "")
        {
          std::cerr << "Found empty var at position " << i << " of Internals->CellVars ";
          break;
        }
        this->GotMask = true;
        mask_pos = i;
        break;
      }
    }
  }

  if (this->GotMask)
  {
    const double maskVal = UseCustomMaskValue
      ? this->CustomMaskValue
      : vlistInqVarMissval(this->VListID, this->Internals->CellVars[mask_pos].VarID);

    CDIVar* cdiVar = &(this->Internals->CellVars[mask_pos]);
    if (this->ShowMultilayerView)
    {
      this->CellMask.resize(this->MaximumCells * this->bloat);
      float* dataTmpMask = new float[this->MaximumCells * sizeof(float)];
      CHECK_NEW(dataTmpMask);

      cdi_set_cur(cdiVar, 0, 0);
      cdi_get_part<float>(cdiVar, this->BeginCell, this->NumberLocalCells, dataTmpMask,
        this->MaximumNVertLevels, this->Grib);
      vtkDebugMacro("Done with read of 3d Mask data");

      // readjust the data
      for (int j = 0; j < this->NumberLocalCells; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          this->CellMask[i + levelNum] =
            (dataTmpMask[j + (levelNum * this->NumberLocalCells)] == maskVal);
        }
      }

      delete[] dataTmpMask;
      vtkDebugMacro("Got data for missing value mask (3D)" << endl);
    }
    else
    {
      this->CellMask.resize(this->NumberLocalCells * this->bloat);
      float* dataTmpMask = new float[this->NumberLocalCells];

      cdi_set_cur(cdiVar, 0, this->VerticalLevelSelected);
      cdi_get_part<float>(
        cdiVar, this->BeginCell, this->NumberLocalCells, dataTmpMask, 1, this->Grib);

      // readjust the data
      for (int j = 0; j < this->NumberLocalCells; j++)
      {
        this->CellMask[j] = (dataTmpMask[j] == maskVal);
      }

      delete[] dataTmpMask;
      vtkDebugMacro("Got data for missing value mask (2D)" << endl);
    }
    this->GotMask = true;
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Add halo to the land sea mask (for points on 180 deg line / ... )
//----------------------------------------------------------------------------
int vtkCDIReader::AddMaskHalo()
{
  if ((this->GotMask) && (this->NumberLocalCells < this->CurrentExtraCell))
  {
    if (this->ShowMultilayerView)
    {
      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int l = j * this->MaximumNVertLevels;
          int k = this->MaximumNVertLevels * this->CellMap[j - this->NumberLocalCells];
          this->CellMask[l + levelNum] = (CellMask[k + levelNum]);
        }
      }
    }
    else
    {
      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        int k = this->CellMap[j - this->NumberLocalCells];
        this->CellMask[j] = this->CellMask[k];
      }
    }

    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
//  Load domain (performance data) variables and integrate them as
//  cell vars. //  Check http://www.kitware.com/blog/home/post/817 if it
//  is possible to adapt to this technique.
//----------------------------------------------------------------------------
bool vtkCDIReader::BuildDomainCellVars()
{
  vtkUnstructuredGrid* output = this->Output;
  this->DomainCellVar = new double[this->NumberOfCells * this->NumberOfDomainVars];
  double* domainTMP = new double[this->NumberOfCells];
  CHECK_NEW(this->DomainCellVar);
  CHECK_NEW(domainTMP);
  double val = 0;
  int mask_pos = 0;
  int numVars = vlistNvars(this->VListID);

  for (int i = 0; i < numVars; i++)
  {
    if (!strcmp(this->Internals->CellVars[i].Name, this->DomainVarName.c_str()))
    {
      mask_pos = i;
    }
  }

  CDIVar* cdiVar = &(this->Internals->CellVars[mask_pos]);
  cdi_set_cur(cdiVar, 0, 0);
  cdi_get_part<double>(cdiVar, this->BeginCell, this->NumberLocalCells, domainTMP, 1, this->Grib);

  for (int j = 0; j < this->NumberOfDomainVars; j++)
  {
    vtkDoubleArray* domainVar = vtkDoubleArray::New();
    for (int k = 0; k < this->NumberOfCells; k++)
    {
      val = this->DomainVarDataArray[j]->GetComponent(domainTMP[k], 0l);
      this->DomainCellVar[k + (j * this->NumberOfCells)] = val;
    }
    domainVar->SetArray(this->DomainCellVar + (j * this->NumberOfCells), this->NumberLocalCells, 0,
      vtkDoubleArray::VTK_DATA_ARRAY_FREE);
    domainVar->SetName(this->Internals->DomainVars[j].c_str());
    output->GetCellData()->AddArray(domainVar);
  }

  delete[] domainTMP;
  vtkDebugMacro("Built cell vars from domain data" << endl);
  return 1;
}

//------------------------------------------------------------------------------
//  Add a "mirror point" X -- a point on the opposite side of the lat/lon projection.
//------------------------------------------------------------------------------

int vtkCDIReader::AddMirrorPointX(int index, double dividerX, double offset)
{
  vtkDebugMacro("In AddMirrorPoint X..." << endl);
  double X = this->PointX[index];
  double Y = this->PointY[index];

  // add on east
  if (X < dividerX)
  {
    X += offset;
  }
  else
  {
    // add on west
    X -= offset;
  }

  //  assert(this->CurrentExtraPoint < this->ModNumPoints);
  this->PointX[this->CurrentExtraPoint] = X;
  this->PointY[this->CurrentExtraPoint] = Y;

  size_t mirrorPoint = this->CurrentExtraPoint;

  // record mapping
  this->PointMap[this->CurrentExtraPoint - this->NumberLocalPoints] = index;
  this->CurrentExtraPoint++;

  vtkDebugMacro("Out AddMirrorPoint X..." << endl);
  return static_cast<int>(mirrorPoint);
}

//------------------------------------------------------------------------------
//  Add a "mirror point" Y -- a point on the opposite side of the Cassini projection.
//------------------------------------------------------------------------------

int vtkCDIReader::AddMirrorPointY(int index, double dividerY, double offset)
{
  vtkDebugMacro("In AddMirrorPoint Y..." << endl);
  double X = this->PointX[index];
  double Y = this->PointY[index];

  // add on south
  if (Y < dividerY)
  {
    Y += offset;
  }
  else
  {
    // add on north
    Y -= offset;
  }

  //  assert(this->CurrentExtraPoint < this->ModNumPoints);
  this->PointX[this->CurrentExtraPoint] = X;
  this->PointY[this->CurrentExtraPoint] = Y;

  size_t mirrorPoint = this->CurrentExtraPoint;

  // record mapping
  this->PointMap[this->CurrentExtraPoint - this->NumberLocalPoints] = index;
  this->CurrentExtraPoint++;

  vtkDebugMacro("Out AddMirrorPoint Y..." << endl);
  return static_cast<int>(mirrorPoint);
}

//----------------------------------------------------------------------------
// Elimate Wrap for lon/lat Projection   1 = x-Axis, 2 = Y-Axis
//----------------------------------------------------------------------------
int vtkCDIReader::Wrap(int axis)
{
  vtkDebugMacro("In Wrapping..." << endl);

  if ((this->WrapOn) &&
    ((this->ProjectionMode == projection::CYLINDRICAL_EQUIDISTANT) ||
      (this->ProjectionMode == projection::CASSINI)))
  {
    double xyLength = 2 * vtkMath::Pi();
    double xyCenter = 0.0;
    const double toleranceXY = 1.0;

    // For each cell, examine vertices
    // Add new points and cells where needed to account for wraparound.
    for (size_t j = 0; j < this->NumberLocalCells; j++)
    {
      int* conns = &this->OrigConnections[j * this->PointsPerCell];
      int* modConns = &this->ModConnections[j * this->PointsPerCell];

      // Determine if we are wrapping in X direction
      size_t lastk = this->PointsPerCell - 1;
      bool xyWrap = false;
      for (size_t k = 0; k < this->PointsPerCell; k++)
      {
        if ((std::abs(this->PointX[conns[k]] - this->PointX[conns[lastk]]) > toleranceXY) &&
          (axis == 1))
        {
          xyWrap = true;
        }
        if ((std::abs(this->PointY[conns[k]] - this->PointY[conns[lastk]]) > toleranceXY) &&
          (axis == 2))
        {
          xyWrap = true;
        }
        lastk = k;
      }

      // If we wrapped in X direction, modify cell and add mirror cell
      if (xyWrap)
      {
        // first point is anchor it doesn't move
        double anchorX = this->PointX[conns[0]];
        double anchorY = this->PointY[conns[0]];

        modConns[0] = conns[0];
        int numExtraPoints = 0;

        // modify existing cell, so it doesn't wrap
        // move points to one side
        for (size_t k = 1; k < this->PointsPerCell; k++)
        {
          int neigh = conns[k];

          // add a new point, figure out east or west
          if ((axis == 1) && (std::abs(this->PointX[neigh] - anchorX) > toleranceXY))
          {
            modConns[k] = this->AddMirrorPointX(neigh, anchorX, xyLength);
            numExtraPoints++;
          }
          else if ((axis == 2) && (std::abs(this->PointY[neigh] - anchorY) > toleranceXY))
          {
            modConns[k] = this->AddMirrorPointY(neigh, anchorY, xyLength);
            numExtraPoints++;
          }
          else
          {
            // use existing kth point
            modConns[k] = neigh;
          }
        }

        // move addedConns to this->ModConnections extra cells area
        int* addedConns = &this->ModConnections[this->CurrentExtraCell * this->PointsPerCell];

        // add a mirroring cell to other side
        // add mirrored anchor first
        if (axis == 1)
        {
          addedConns[0] = this->AddMirrorPointX(conns[0], xyCenter, xyLength);
          anchorX = this->PointX[addedConns[0]];
        }
        else if (axis == 2)
        {
          addedConns[0] = this->AddMirrorPointY(conns[0], xyCenter, xyLength);
          anchorY = this->PointY[addedConns[0]];
        }
        numExtraPoints++;

        // add mirror cell points if needed
        for (size_t k = 1; k < this->PointsPerCell; k++)
        {
          int neigh = conns[k];

          // add a new point for neighbor, figure out east or west
          if ((axis == 1) && (std::abs(this->PointX[neigh] - anchorX) > toleranceXY))
          {
            addedConns[k] = this->AddMirrorPointX(neigh, anchorX, xyLength);
            numExtraPoints++;
          }
          else if ((axis == 2) && (std::abs(this->PointY[neigh] - anchorY) > toleranceXY))
          {
            addedConns[k] = this->AddMirrorPointY(neigh, anchorY, xyLength);
            numExtraPoints++;
          }
          else
          {
            // use existing kth point
            addedConns[k] = neigh;
          }
        }
        this->CellMap[this->CurrentExtraCell - this->NumberLocalCells] = j;
        this->CurrentExtraCell++;
      }
      else
      {
        // just add cell "as is" to this->ModConnections
        for (size_t k = 0; k < this->PointsPerCell; k++)
        {
          modConns[k] = conns[k];
        }
      }
      if (this->CurrentExtraCell > this->ModNumCells)
      {
        vtkErrorMacro(<< "Exceeded storage for extra cells!" << endl);
        return (0);
      }
      if (this->CurrentExtraPoint > this->ModNumPoints)
      {
        vtkErrorMacro(<< "Exceeded storage for extra points!" << endl);
        return (0);
      }
    }

    if (!ShowMultilayerView)
    {
      this->MaximumCells = static_cast<int>(this->CurrentExtraCell);
      this->MaximumPoints = static_cast<int>(this->CurrentExtraPoint);
      vtkDebugMacro(<< "elim xwrap: singlelayer: setting this->MaximumPoints to "
                    << this->MaximumPoints << endl);
    }
    else
    {
      this->MaximumCells = static_cast<int>(this->CurrentExtraCell * this->MaximumNVertLevels);
      this->MaximumPoints =
        static_cast<int>(this->CurrentExtraPoint * (this->MaximumNVertLevels + 1));
      vtkDebugMacro(<< "elim xwrap: multilayer: setting this->MaximumPoints to "
                    << this->MaximumPoints << endl);
    }

    vtkDebugMacro("Out Wrapping..." << endl);
    return 1;
  }
  else
  {
    for (int j = 0; j < this->NumberLocalCells; j++)
    {
      int* conns = &this->OrigConnections[j * this->PointsPerCell];
      int* modConns = &this->ModConnections[j * this->PointsPerCell];
      int lastk = this->PointsPerCell - 1;
      bool xyWrap = false;

      for (int k = 0; k < this->PointsPerCell; k++)
      {
        if ((abs(this->PointX[conns[k]] - this->PointX[conns[lastk]]) > 1.0) && (axis == 1))
        {
          xyWrap = true;
        }
        if ((abs(this->PointY[conns[k]] - this->PointY[conns[lastk]]) > 1.0) && (axis == 2))
        {
          xyWrap = true;
        }

        lastk = k;
      }

      if (xyWrap)
      {
        for (int k = 0; k < this->PointsPerCell; k++)
        {
          modConns[k] = 0;
        }
      }
      else
      {
        for (int k = 0; k < this->PointsPerCell; k++)
        {
          modConns[k] = conns[k];
        }
      }
    }
    vtkDebugMacro("Out Wrapping..." << endl);
    return 1;
  }
}

//----------------------------------------------------------------------------
//  Add points to vtk data structures
//----------------------------------------------------------------------------
void vtkCDIReader::OutputPoints(bool init)
{
  vtkDebugMacro("In OutputPoints..." << endl);
  float layerThicknessScaleFactor = 5000.0;
  vtkUnstructuredGrid* output = this->Output;
  vtkSmartPointer<vtkPoints> points;
  float adjustedLayerThickness = (this->LayerThickness / layerThicknessScaleFactor);

  if (this->InvertZAxis)
  {
    adjustedLayerThickness = -1.0 * (this->LayerThickness / layerThicknessScaleFactor);
  }

  vtkDebugMacro("OutputPoints: this->MaximumPoints: "
    << this->MaximumPoints << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
    << " LayerThickness: " << this->LayerThickness
    << " ShowMultilayerView: " << this->ShowMultilayerView << endl);
  if (init)
  {
    points = vtkSmartPointer<vtkPoints>::New();
    points->Allocate(this->MaximumPoints, this->MaximumPoints);
    output->SetPoints(points);
  }
  else
  {
    points = output->GetPoints();
    points->Initialize();
    points->Allocate(this->MaximumPoints, this->MaximumPoints);
  }

  if (this->DepthVar == nullptr)
  {
    vtkDebugMacro("OutputPoints: this->MaximumPoints: "
      << this->MaximumPoints << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
      << " LayerThickness: " << this->LayerThickness
      << " ShowMultilayerView: " << this->ShowMultilayerView << endl);
    return; // We don't have any data anyways.
  }

  double sx, sy, sz;
  get_scaling(this->ProjectionMode, this->ShowMultilayerView,
    this->DepthVar[this->VerticalLevelSelected], adjustedLayerThickness, sx, sy, sz);

  for (int j = 0; j < this->CurrentExtraPoint; j++)
  {
    double x = this->PointX[j] * sx;
    double y = this->PointY[j] * sy;
    double z = this->PointZ[j] * sz;

    if (!this->ShowMultilayerView)
    {
      points->InsertNextPoint(x, y, z);
    }
    else
    {
      double rho = 0.0, rholevel = 0.0, theta = 0.0, phi = 0.0;
      int retval = -1;

      if (this->ProjectionMode == projection::SPHERICAL)
      {
        if ((x != 0.0) || (y != 0.0) || (z != 0.0))
        {
          retval = ::CartesianToSpherical(x, y, z, &rho, &phi, &theta);
          if (!retval)
            retval = ::SphericalToCartesian(
              rho + this->Layer0Offset * adjustedLayerThickness, phi, theta, &x, &y, &z);
        }
      }

      if (this->ProjectionMode != projection::SPHERICAL)
      {
        z = this->Layer0Offset * adjustedLayerThickness; // to avoid 0 layer thickness / ...
      }

      points->InsertNextPoint(x, y, z);

      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        if ((this->ProjectionMode != projection::SPHERICAL) &&
          (this->ProjectionMode != projection::CATALYST))
        {
          z = -(this->DepthVar[levelNum] * adjustedLayerThickness);
        }
        else if (this->ProjectionMode == projection::SPHERICAL)
        {
          if (!retval && ((x != 0.0) || (y != 0.0) || (z != 0.0)))
          {
            rholevel = rho - (adjustedLayerThickness * this->DepthVar[levelNum]);
            retval = ::SphericalToCartesian(rholevel, phi, theta, &x, &y, &z);
          }
        }
        else if (this->ProjectionMode == projection::CATALYST)
        {
          z = -(this->DepthVar[levelNum] * (adjustedLayerThickness * 0.04));
        }
        points->InsertNextPoint(x, y, z);
      }
    }
  }
  if (abs(this->DepthVar[0] - this->Layer0Offset) < 1 && this->ShowMultilayerView)
  {
    vtkWarningMacro(<< this->FileName << ": First vertical level thickness is close to 0 ("
                    << abs(this->DepthVar[0] - this->Layer0Offset)
                    << " to be precise). It will be rendered as basically flat. Use \"3D Surface Z "
                       "offset for first Layer\" to adjust. Current value is "
                    << this->Layer0Offset << ".");
  }
  vtkDebugMacro("Leaving OutputPoints..." << endl);
}

//----------------------------------------------------------------------------
// Determine if cell is one of VTK_TRIANGLE, VTK_WEDGE, VTK_QUAD or
// VTK_HEXAHEDRON
//----------------------------------------------------------------------------
unsigned char vtkCDIReader::GetCellType()
{
  // write cell types
  switch (this->PointsPerCell)
  {
    case 3:
      return (!this->ShowMultilayerView) ? VTK_TRIANGLE : VTK_WEDGE;
    case 4:
      return (!this->ShowMultilayerView) ? VTK_QUAD : VTK_HEXAHEDRON;
    case 5:
      return (!this->ShowMultilayerView) ? VTK_POLYGON : VTK_PENTAGONAL_PRISM;
    case 6:
      return (!this->ShowMultilayerView) ? VTK_POLYGON : VTK_HEXAGONAL_PRISM;
    default:
      return (!this->ShowMultilayerView) ? VTK_POLYGON : VTK_POLYHEDRON;
  }
  return VTK_TRIANGLE;
}

//----------------------------------------------------------------------------
//  Add cells to vtk data structures
//----------------------------------------------------------------------------
void vtkCDIReader::OutputCells(bool init)
{
  vtkDebugMacro("In OutputCells..." << endl);
  vtkUnstructuredGrid* output = this->Output;

  output->GetCellData()->Initialize();
  output->GetCellData()->SetNumberOfTuples(this->MaximumCells);
  output->Allocate(this->MaximumCells, this->MaximumCells);

  vtkSmartPointer<vtkCellArray> cells;
  int pointsPerPolygon = this->PointsPerCell * (this->ShowMultilayerView ? 2 : 1);
  int cellType = this->GetCellType();

  if (init)
  {
    cells = vtkSmartPointer<vtkCellArray>::New();
    cells->Allocate(this->MaximumCells, this->MaximumCells);
    output->SetCells(cellType, cells);
    cells->SetNumberOfCells(this->MaximumCells);
  }
  else
  {
    cells = output->GetCells();
    cells->Initialize();
    cells->Allocate(this->MaximumCells, this->MaximumCells);
    cells->SetNumberOfCells(this->MaximumCells);
  }

  vtkDebugMacro("OutputCells: init: "
    << init << " this->MaximumCells: " << this->MaximumCells << " cellType: " << cellType
    << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
    << " LayerThickness: " << LayerThickness << " ShowMultilayerView: " << this->ShowMultilayerView
    << " CurrentExtraCell: " << this->CurrentExtraCell);

  if (this->DepthVar == nullptr)
  {
    vtkErrorMacro("File " << this->FileName
                          << " OutputCells: this->MaximumCells: " << this->MaximumCells
                          << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
                          << " LayerThickness: " << this->LayerThickness
                          << " ShowMultilayerView: " << this->ShowMultilayerView << endl);
    return; // We don't have any data anyways.
  }

  std::vector<vtkIdType> polygon(pointsPerPolygon);
  for (int j = 0; j < this->CurrentExtraCell; j++)
  {
    int* conns;
    if (this->ProjectionMode != projection::SPHERICAL)
    {
      conns = &this->ModConnections[j * this->PointsPerCell];
    }
    else
    {
      conns = &this->OrigConnections[j * this->PointsPerCell];
    }

    // check for outer boundary cell
    bool boundary = false;
    if (this->ProjectionMode == projection::CYLINDRICAL_EQUIDISTANT ||
      this->ProjectionMode == projection::CASSINI)
    {

      double borderX, borderY;
      if (this->ProjectionMode == projection::CYLINDRICAL_EQUIDISTANT)
      {
        borderX = vtkMath::Pi() + 0.5;
        borderY = (vtkMath::Pi() / 2.0) + 0.5;
      }
      else
      {
        borderX = (vtkMath::Pi() / 2.0) + 0.5;
        borderY = vtkMath::Pi() + 0.5;
      }
      for (size_t k = 0; k < this->PointsPerCell; k++)
      {
        double X = this->PointX[conns[k]];
        double Y = this->PointY[conns[k]];
        if (((X > borderX) || (X < (0.0 - borderX)) || (Y > borderY) || (Y < (0.0 - borderY))))
        {
          boundary = true;
        }
      }
    }

    if (!this->ShowMultilayerView)
    { // singlelayer
      if (((this->GotMask) && (this->UseMask) && (this->CellMask[j] ^ this->InvertMask)) ||
        boundary)
      {
        output->InsertNextCell(VTK_EMPTY_CELL, 0, polygon.data());
      }
      else
      {
        for (int k = 0; k < this->PointsPerCell; k++)
        {
          polygon[k] = conns[k];
        }

        output->InsertNextCell(cellType, pointsPerPolygon, polygon.data());
      }
    }
    else
    { // multilayer
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        int i = j * this->MaximumNVertLevels;
        if (((this->GotMask) && (this->UseMask) && (this->CellMask[i + levelNum])) || boundary)
        {
          output->InsertNextCell(VTK_EMPTY_CELL, 0, polygon.data());
        }
        else
        {
          for (int k = 0; k < this->PointsPerCell; k++)
          {
            int val = (conns[k] * (this->MaximumNVertLevels + 1)) + levelNum;
            polygon[k] = val;
          }
          for (int k = 0; k < this->PointsPerCell; k++)
          {
            int val = (conns[k] * (this->MaximumNVertLevels + 1)) + levelNum + 1;
            polygon[k + this->PointsPerCell] = val;
          }
          if (cellType == VTK_POLYHEDRON)
            this->InsertPolyhedron(polygon);
          else
            output->InsertNextCell(cellType, pointsPerPolygon, polygon.data());
        }
      }
    }
  }

  if (this->AddCoordinateVars && this->ShowClonClat)
  {
    this->clonArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->clonArray->SetName("Center Longitude (CLON)");
    this->clonArray->SetNumberOfTuples(this->NumberLocalCells * this->MaximumNVertLevels);
    this->clatArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->clatArray->SetName("Center Latitude (CLAT)");
    this->clatArray->SetNumberOfTuples(this->NumberLocalCells * this->MaximumNVertLevels);
    if (this->ShowMultilayerView)
    {
      this->clonArray->SetArray(
        &this->CLon[0], this->CurrentExtraCell * this->MaximumNVertLevels, 1);
      this->clatArray->SetArray(
        &this->CLat[0], this->CurrentExtraCell * this->MaximumNVertLevels, 1);
    }
    else
    {
      this->clonArray->SetArray(&this->CLon[0], this->CurrentExtraCell,
        1); // 1 at the end = No freeing these. They are our vectors.
      this->clatArray->SetArray(&this->CLat[0], this->CurrentExtraCell,
        1); // 1 at the end = No freeing these. They are our vectors.
    }
    output->GetCellData()->AddArray(this->clonArray);
    output->GetCellData()->AddArray(this->clatArray);
  }

  /*
    if (this->GotMask) // Does not work for bool. Complete shit show, vector<bool> is different and
    SetArray expects unsigned char for a bit array.
    {
      this->maskArray = vtkSmartPointer<vtkIntArray>::New();
      this->maskArray->SetName("Land/Sea Mask (wet_c)");
      this->maskArray->SetNumberOfTuples(this->CurrentExtraCell * this->MaximumNVertLevels);
      if (this->ShowMultilayerView)
      {
        this->maskArray->SetArray(this->CellMask, this->CurrentExtraCell * this->MaximumNVertLevels,
    0, vtkIntArray::VTK_DATA_ARRAY_FREE);
      }
      else
      {
        this->maskArray->SetArray(this->CellMask, this->CurrentExtraCell, 0,
    vtkIntArray::VTK_DATA_ARRAY_FREE);
      }
      output->GetCellData()->AddArray(this->maskArray);
    }
  */

  vtkDebugMacro("Leaving OutputCells..." << endl);
}

void vtkCDIReader::InsertPolyhedron(std::vector<vtkIdType> polygon)
{

  std::vector<vtkIdType>::iterator it;
  it = std::unique(polygon.begin(), polygon.end());
  polygon.resize(std::distance(polygon.begin(), it));
  const int pointsPerPolygon = polygon.size();

  vtkUnstructuredGrid* output = this->Output;
  const int pointsPerCell = pointsPerPolygon / 2;
  const int numFaces = pointsPerCell + 2;
  std::vector<vtkIdType> facestream(7 * pointsPerCell + 2);
  // number of points of the face, then the points of the face
  // 1 + pointsPerCell values for lid / bottom
  // 1 + 4 values for each of the pointsPerCell side faces
  // bottom
  size_t next = 0;
  facestream[next++] = pointsPerCell;
  for (int k = 0; k < pointsPerCell; k++)
  {
    facestream[next++] = polygon[pointsPerCell - 1 - k];
  }
  // lid
  facestream[next++] = pointsPerCell;
  for (int k = pointsPerCell; k < pointsPerCell * 2; k++)
  {
    facestream[next++] = polygon[k];
  }
  for (int k = 0; k < pointsPerCell - 1; k++)
  {
    facestream[next++] = 4;
    facestream[next++] = polygon[k];
    facestream[next++] = polygon[k + 1];
    facestream[next++] = polygon[k + 1 + pointsPerCell];
    facestream[next++] = polygon[k + pointsPerCell];
  }
  facestream[next++] = 4;
  facestream[next++] = polygon[pointsPerCell - 1];
  facestream[next++] = polygon[0];
  facestream[next++] = polygon[pointsPerCell];
  facestream[next++] = polygon[pointsPerCell * 2 - 1];
  output->InsertNextCell(
    VTK_POLYHEDRON, pointsPerPolygon, polygon.data(), numFaces, facestream.data());
};

//----------------------------------------------------------------------------
//  Load the data for a Point variable specified.
//----------------------------------------------------------------------------
int vtkCDIReader::LoadPointVarData(int variableIndex, double dTimeStep)
{
  if (!(this->PointsPerCell == 3))
    return 0;

  this->PointDataSelected = variableIndex;

  vtkDataArray* dataArray = this->PointVarDataArray[variableIndex];

  // Allocate data array for this variable
  if (dataArray == nullptr)
  {
    dataArray = this->DoublePrecision ? static_cast<vtkDataArray*>(vtkDoubleArray::New())
                                      : static_cast<vtkDataArray*>(vtkFloatArray::New());

    vtkDebugMacro(
      "Allocated Point var index: " << this->Internals->PointVars[variableIndex].Name << endl);
    dataArray->SetName(this->Internals->PointVars[variableIndex].Name);
    dataArray->SetNumberOfTuples(this->MaximumPoints);
    dataArray->SetNumberOfComponents(1);

    this->PointVarDataArray[variableIndex] = dataArray;
  }

  int success = false;
  if (this->DoublePrecision)
  {
    vtkICONTemplateDispatch(
      VTK_DOUBLE,
      success = this->LoadPointVarDataTemplate<VTK_TT>(variableIndex, dTimeStep, dataArray););
  }
  else
  {
    vtkICONTemplateDispatch(
      VTK_FLOAT,
      success = this->LoadPointVarDataTemplate<VTK_TT>(variableIndex, dTimeStep, dataArray););
  }

  return success;
}

//----------------------------------------------------------------------------
//  Load the data for a cell variable specified.
//----------------------------------------------------------------------------
int vtkCDIReader::LoadCellVarData(int variableIndex, double dTimeStep)
{
  this->CellDataSelected = variableIndex;

  vtkDataArray* dataArray = this->CellVarDataArray[variableIndex];
  // Allocate data array for this variable
  if (dataArray == nullptr)
  {
    dataArray = this->DoublePrecision ? static_cast<vtkDataArray*>(vtkDoubleArray::New())
                                      : static_cast<vtkDataArray*>(vtkFloatArray::New());

    vtkDebugMacro(
      "Allocated cell var index: " << this->Internals->CellVars[variableIndex].Name << endl);
    dataArray->SetName(this->Internals->CellVars[variableIndex].Name);
    dataArray->SetNumberOfTuples(this->MaximumCells);
    dataArray->SetNumberOfComponents(1);

    this->CellVarDataArray[variableIndex] = dataArray;
  }

  int success = false;
  if (this->DoublePrecision)
  {
    vtkICONTemplateDispatch(
      VTK_DOUBLE,
      success = this->LoadCellVarDataTemplate<VTK_TT>(variableIndex, dTimeStep, dataArray););
  }
  else
  {
    vtkICONTemplateDispatch(
      VTK_FLOAT,
      success = this->LoadCellVarDataTemplate<VTK_TT>(variableIndex, dTimeStep, dataArray););
  }

  return success;
}

//----------------------------------------------------------------------------
//  Load the data for a cell variable specified.
//----------------------------------------------------------------------------
template <typename ValueType>
int vtkCDIReader::LoadCellVarDataTemplate(
  int variableIndex, double dTimeStep, vtkDataArray* dataArray)
{
  vtkDebugMacro("In vtkCDIReader::LoadCellVarData" << endl);
  ValueType* dataBlock = static_cast<ValueType*>(dataArray->GetVoidPointer(0));
  CDIVar* cdiVar = &(this->Internals->CellVars[variableIndex]);
  int varType = cdiVar->Type;

  int Timestep = this->getTimeIndex(dTimeStep);
  vtkDebugMacro("Time: " << Timestep << endl);
  vtkDebugMacro("Dimensions: " << varType << endl);

  if (varType == 3) // 3D arrays
  {
    if (!this->ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, Timestep, this->VerticalLevelSelected);
      cdi_get_part<ValueType>(
        cdiVar, this->BeginCell, this->NumberLocalCells, dataBlock, 1, this->Grib);

      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        int k = this->CellMap[j - this->NumberLocalCells];
        dataBlock[j] = dataBlock[k];
      }
    }
    else
    {
      ValueType* dataTmp = new ValueType[this->MaximumCells];
      cdi_set_cur(cdiVar, Timestep, 0);
      cdi_get_part<ValueType>(cdiVar, this->BeginCell, this->NumberLocalCells, dataTmp,
        this->MaximumNVertLevels, this->Grib);

      // readjust the data
      for (int j = 0; j < this->NumberLocalCells; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          dataBlock[i + levelNum] = dataTmp[j + (levelNum * this->NumberLocalCells)];
        }
      }

      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int l = j * this->MaximumNVertLevels;
          int k = this->CellMap[j - this->NumberLocalCells];
          dataBlock[l + levelNum] = dataTmp[k + (levelNum * this->NumberLocalCells)];
        }
      }

      delete[] dataTmp;
    }
    vtkDebugMacro(
      "Got data for cell var: " << this->Internals->CellVars[variableIndex].Name << endl);
  }
  else // 2D arrays
  {
    if (!this->ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, Timestep, 0);
      cdi_get_part<ValueType>(
        cdiVar, this->BeginCell, this->NumberLocalCells, dataBlock, 1, this->Grib);

      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        int k = this->CellMap[j - this->NumberLocalCells];
        dataBlock[j] = dataBlock[k];
      }
    }
    else
    {
      ValueType* dataTmp = new ValueType[this->NumberLocalCells];
      cdi_set_cur(cdiVar, Timestep, 0);
      cdi_get_part<ValueType>(
        cdiVar, this->BeginCell, this->NumberLocalCells, dataTmp, 1, this->Grib);

      for (int j = 0; j < +this->NumberLocalCells; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          dataBlock[i + levelNum] = dataTmp[j];
        }
      }

      // put out data for extra cells
      for (int j = this->NumberLocalCells; j < this->CurrentExtraCell; j++)
      {
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int l = j * this->MaximumNVertLevels;
          int k = this->CellMap[j - this->NumberLocalCells];
          dataBlock[l + levelNum] = dataTmp[k];
        }
      }

      delete[] dataTmp;
    }

    vtkDebugMacro(
      "Got data for cell var: " << this->Internals->CellVars[variableIndex].Name << endl);
  }

  vtkDebugMacro(
    "Stored data for cell var: " << this->Internals->CellVars[variableIndex].Name << endl);

  ReplaceFillWithNan(cdiVar->VarID, dataArray);
  return 1;
}

int vtkCDIReader::ReplaceFillWithNan(const int varID, vtkDataArray* dataArray)
{

  if (true) //  (this->ReplaceFillValueWithNan)
  {

    double miss = vlistInqVarMissval(this->VListID, varID);

    // NaN only available with float and double.
    if (dataArray->GetDataType() == VTK_FLOAT)
    {

      float fillValue = miss;
      //        nc_get_att_float(ncFD, varId, "_FillValue", &fillValue);
      std::replace(reinterpret_cast<float*>(dataArray->GetVoidPointer(0)),
        reinterpret_cast<float*>(dataArray->GetVoidPointer(dataArray->GetNumberOfTuples())),
        fillValue, static_cast<float>(vtkMath::Nan()));
    }
    else if (dataArray->GetDataType() == VTK_DOUBLE)
    {
      double fillValue = miss;
      //        nc_get_att_double(ncFD, varId, "_FillValue", &fillValue);
      std::replace(reinterpret_cast<double*>(dataArray->GetVoidPointer(0)),
        reinterpret_cast<double*>(dataArray->GetVoidPointer(dataArray->GetNumberOfTuples())),
        fillValue, vtkMath::Nan());
    }
    else
    {
      vtkWarningMacro(<< "No NaN available for data of type " << dataArray->GetDataType());
    }

    return 1;
  }
}
//----------------------------------------------------------------------------
//  Load the data for a Point variable specified.
//----------------------------------------------------------------------------
template <typename ValueType>
int vtkCDIReader::LoadPointVarDataTemplate(
  int variableIndex, double dTimeStep, vtkDataArray* dataArray)
{
  vtkDebugMacro("In vtkICONReader::LoadPointVarData" << endl);
  CDIVar* cdiVar = &this->Internals->PointVars[variableIndex];
  int varType = cdiVar->Type;

  vtkDebugMacro("getting pointer in vtkICONReader::LoadPointVarData" << endl);
  ValueType* dataBlock = static_cast<ValueType*>(dataArray->GetVoidPointer(0));
  ValueType* dataTmp;
  if (this->ShowMultilayerView)
  {
    dataTmp = new ValueType[this->MaximumPoints];
  }
  else
  {
    dataTmp = new ValueType[this->NumberLocalPoints];
  }

  int Timestep = this->getTimeIndex(dTimeStep);
  vtkDebugMacro("Time: " << Timestep << endl);
  vtkDebugMacro("dTimeStep requested: " << dTimeStep << endl);

  if (this->Piece < 1)
  {
    // 3D arrays ...
    vtkDebugMacro("Dimensions: " << varType << endl);
    if (varType == 3)
    {
      if (!this->ShowMultilayerView)
      {
        cdi_set_cur(cdiVar, Timestep, this->VerticalLevelSelected);
        cdi_get_part<ValueType>(
          cdiVar, this->BeginPoint, this->NumberLocalPoints, dataBlock, 1, this->Grib);
        dataBlock[0] = dataBlock[1];

        // put out data for extra points
        for (int j = this->NumberLocalPoints; j < this->CurrentExtraPoint; j++)
        {
          int k = this->PointMap[j - this->NumberLocalPoints];
          dataBlock[j] = dataBlock[k];
        }
      }
      else
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(cdiVar, this->BeginPoint, this->NumberLocalPoints, dataTmp,
          this->MaximumNVertLevels, this->Grib);
        dataTmp[0] = dataTmp[1];

        // put out data for extra points
        for (int j = this->NumberLocalPoints; j < this->CurrentExtraPoint; j++)
        {
          for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
          {
            int l = j * this->MaximumNVertLevels;
            int k = this->PointMap[j - this->NumberLocalPoints];
            dataBlock[l + levelNum] = dataTmp[k + (levelNum * this->NumberLocalPoints)];
          }
        }
      }
    }
    // 2D arrays ...
    else if (varType == 2)
    {
      if (!this->ShowMultilayerView)
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(
          cdiVar, this->BeginPoint, this->NumberLocalPoints, dataBlock, 1, this->Grib);
        dataBlock[0] = dataBlock[1];
      }
      else
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(
          cdiVar, this->BeginPoint, this->NumberLocalPoints, dataTmp, 1, this->Grib);
        dataTmp[0] = dataTmp[1];
      }
    }
    vtkDebugMacro("got Point data in vtkICONReader::LoadPointVarDataSP" << endl);

    if (this->ShowMultilayerView)
    {
      // put in some dummy points
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        dataBlock[levelNum] = dataTmp[this->MaximumNVertLevels + levelNum];
      }

      // write highest level dummy Point (duplicate of last level)
      dataBlock[this->MaximumNVertLevels] =
        dataTmp[this->MaximumNVertLevels + this->MaximumNVertLevels - 1];
      vtkDebugMacro("Wrote dummy vtkICONReader::LoadPointVarDataSP" << endl);

      // readjust the data
      for (int j = 0; j < this->NumberLocalPoints; j++)
      {
        int i = j * (this->MaximumNVertLevels + 1);
        // write data for one Point -- lowest level to highest
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          dataBlock[i++] = dataTmp[j + (levelNum * this->NumberLocalPoints)];
        }

        // layer below, which is repeated ...
        dataBlock[i++] = dataTmp[j + ((MaximumNVertLevels - 1) * this->NumberLocalPoints)];
      }
    }
  }
  else
  {
    int length = (this->NumberAllPoints / this->NumPieces);
    int start = (length * this->Piece);
    ValueType* dataTmp2 = new ValueType[length];

    // 3D arrays ...
    vtkDebugMacro("Dimensions: " << varType << endl);
    if (varType == 3)
    {
      if (!this->ShowMultilayerView)
      {
        cdi_set_cur(cdiVar, Timestep, this->VerticalLevelSelected);
        cdi_get_part<ValueType>(cdiVar, start, length, dataTmp2, 1, this->Grib);
        dataTmp2[0] = dataTmp2[1];

        // readjust the data
        size_t size = this->NumberLocalCells * this->PointsPerCell;
        for (size_t j = 0; j < size; j++)
        {
          int pos = this->VertexIds[j];
          int pos_conn = this->OrigConnections[j];
          if ((pos > start) && (pos < (start + length + 1)))
          {
            dataBlock[pos_conn] = dataTmp2[pos - start];
          }
          else
          {
            dataBlock[pos_conn] = 0.0;
          }
        }
      }
      else
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(
          cdiVar, start, length, dataTmp, this->MaximumNVertLevels, this->Grib);
        dataTmp[0] = dataTmp[1];
      }
    }
    // 2D arrays ...
    else if (varType == 2)
    {
      if (!this->ShowMultilayerView)
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(cdiVar, start, length, dataTmp2, 1, this->Grib);
        dataTmp2[0] = dataTmp2[1];

        // readjust the data
        size_t size = this->NumberLocalCells * this->PointsPerCell;
        for (size_t j = 0; j < size; j++)
        {
          int pos = this->VertexIds[j];
          int pos_conn = this->OrigConnections[j];
          if ((pos > start) && (pos < (start + length + 1)))
          {
            dataBlock[pos_conn] = dataTmp2[pos - start];
          }
          else
          {
            dataBlock[pos_conn] = 0.0;
          }
        }
      }
      else
      {
        cdi_set_cur(cdiVar, Timestep, 0);
        cdi_get_part<ValueType>(cdiVar, start, length, dataTmp, 1, this->Grib);
        dataTmp[0] = dataTmp[1];
      }
    }
    delete[] dataTmp2;
    vtkDebugMacro("got Point data in vtkICONReader::LoadPointVarDataSP" << endl);
  }

  vtkDebugMacro("this->NumberOfPoints: " << this->NumberOfPoints << " this->NumberLocalPoints: "
                                         << this->NumberLocalPoints << endl);
  delete[] dataTmp;

  return 1;
}

//----------------------------------------------------------------------------
//  Load the data for a domain variable specified
//----------------------------------------------------------------------------
int vtkCDIReader::LoadDomainVarData(int variableIndex)
{
  // This is not very well implemented, also due to the organization of
  // the data available. Needs to be improved together with the modellers.
  vtkDebugMacro("In vtkCDIReader::LoadDomainVarData" << endl);
  string variable = this->Internals->DomainVars[variableIndex];
  this->DomainDataSelected = variableIndex;

  // Allocate data array for this variable
  if (this->DomainVarDataArray[variableIndex] == nullptr)
  {
    this->DomainVarDataArray[variableIndex] = vtkDoubleArray::New();
    vtkDebugMacro("Allocated domain var index: " << variable.c_str() << endl);
    this->DomainVarDataArray[variableIndex]->SetName(variable.c_str());
    this->DomainVarDataArray[variableIndex]->SetNumberOfTuples(this->NumberOfDomains);
    // 6 components
    this->DomainVarDataArray[variableIndex]->SetNumberOfComponents(1);
  }

  for (int i = 0; i < this->NumberOfDomains; i++)
  {
    string filename;
    if (i < 10)
    {
      filename = this->PerformanceDataFile + "000" + ::ConvertInt(i);
    }
    else if (i < 100)
    {
      filename = this->PerformanceDataFile + "00" + ::ConvertInt(i);
    }
    else if (i < 1000)
    {
      filename = this->PerformanceDataFile + "0" + ::ConvertInt(i);
    }
    else
    {
      filename = this->PerformanceDataFile + ::ConvertInt(i);
    }

    vector<string> wordVec;
    vector<string>::iterator k;
    vtksys::ifstream file(filename.c_str());
    string str, word;
    double temp[1];

    for (int j = 0; j < DomainMask[variableIndex]; j++)
    {
      getline(file, str);
    }

    getline(file, str);
    stringstream ss(str);
    while (ss.good())
    {
      ss >> word;
      ::Strip(word);
      wordVec.push_back(word);
    }
    //  0    1      	2     		3      	4       	5      	6 7		  8
    //  th  L 	name   	#calls 	t_min 	t_ave	t_max 	t_total	   t_total2
    // 00   L		physics   251    	0.4222s  0.9178s  10.52s    03m50s     230.21174
    // for (int l=0; l<6 ; l++)
    //  temp[l] = atof(wordVec.at(2+l).c_str());

    if (wordVec.at(1) != "L")
    {
      temp[0] = atof(wordVec.at(7).c_str());
    }
    else
    {
      temp[0] = atof(wordVec.at(8).c_str());
    }

    // for now, we just use t_average
    this->DomainVarDataArray[variableIndex]->InsertTuple(i, temp);
  }

  vtkDebugMacro("Out vtkCDIReader::LoadDomainVarData" << endl);
  return 1;
}

//----------------------------------------------------------------------------
//  Specify the netCDF dimension names
//-----------------------------------------------------------------------------
int vtkCDIReader::FillGridDimensions()
{
  int ngrids = vlistNgrids(this->VListID);
  int nzaxis = vlistNzaxis(this->VListID);
  int nvars = vlistNvars(this->VListID);
  this->AllDimensions->SetNumberOfValues(0);
  this->VariableDimensions->SetNumberOfValues(ngrids * nzaxis);
  char nameGridX[CDI_MAX_NAME];
  char nameGridY[CDI_MAX_NAME];
  char nameLev[CDI_MAX_NAME];

  set<string> hits;

  for (int k = 0; k < nvars; k++)
  {
    int i = vlistInqVarGrid(this->VListID, k);
    int j = vlistInqVarZaxis(this->VListID, k);
    hits.insert(to_string(i) + "x" + to_string(j));
    // IDs are not 0 to n-1 but can be 30-ish for a file with 3 grids.
    // they map to the gridID_l and zaxisID_l values below.
    // Thus we need to a map to catch rather unpredictable values.
  }

  for (int i = 0; i < ngrids; ++i)
  {
    for (int j = 0; j < nzaxis; ++j)
    {
      string dimEncoding("(");
      int gridID_l = vlistGrid(this->VListID, i);
      gridInqXname(gridID_l, nameGridX);
      gridInqYname(gridID_l, nameGridY);
      dimEncoding += nameGridX;
      dimEncoding += ", ";
      dimEncoding += nameGridY;
      dimEncoding += ", ";
      int zaxisID_l = vlistZaxis(this->VListID, j);
      zaxisInqName(zaxisID_l, nameLev);
      dimEncoding += nameLev;
      dimEncoding += ")";

      if (hits.count(to_string(gridID_l) + "x" + to_string(zaxisID_l)) == 0)
      {
        continue; // skip empty grid combinations
      }

      this->AllDimensions->InsertNextValue(dimEncoding);
      this->VariableDimensions->SetValue(i * nzaxis + j, dimEncoding.c_str());
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Set dimensions and rebuild geometry
//-----------------------------------------------------------------------------
void vtkCDIReader::SetDimensions(const char* dimensions)
{
  for (vtkIdType i = 0; i < this->VariableDimensions->GetNumberOfValues(); i++)
  {
    if (this->VariableDimensions->GetValue(i) == dimensions)
    {
      this->DimensionSelection = i;
    }
  }

  this->PointDataArraySelection->RemoveAllArrays();
  this->CellDataArraySelection->RemoveAllArrays();
  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();

  if (this->DomainDataArraySelection)
  {
    this->DomainDataArraySelection->RemoveAllArrays();
    this->DomainDataArraySelection->Modified();
  }

  this->ReconstructNew = true;
  this->DestroyData();
  this->RegenerateVariables();
  if (this->GridReconstructed)
    this->RegenerateGeometry();
}

//----------------------------------------------------------------------------
//  Return all variable names
//-----------------------------------------------------------------------------
vtkStringArray* vtkCDIReader::GetAllVariableArrayNames()
{
  int numArrays = this->GetNumberOfVariableArrays();
  this->AllVariableArrayNames->SetNumberOfValues(numArrays);
  for (int arrayIdx = 0; arrayIdx < numArrays; arrayIdx++)
  {
    const char* arrayName = this->GetVariableArrayName(arrayIdx);
    this->AllVariableArrayNames->SetValue(arrayIdx, arrayName);
  }
  return this->AllVariableArrayNames;
}

//----------------------------------------------------------------------------
// Set status of named Point variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
// Set status of named cell variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
// Set status of named domain variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetDomainArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->DomainDataArraySelection->EnableArray(name);
  }
  else
  {
    this->DomainDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
// Set status of named domain variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetMaskingVariable(const char* name)
{
  vtkDebugMacro("Setting MaskingVariable to " << name << endl);

  MaskingVarname = name;

  this->Modified();

  if (!this->InfoRequested || !this->DataRequested)
  {
    return;
  }
  this->DestroyData();
  this->RegenerateGeometry();
}

//----------------------------------------------------------------------------
// Toggle the Use of the custom mask value.
//----------------------------------------------------------------------------
void vtkCDIReader::SetUseCustomMaskValue(bool val)
{
  if (val == UseCustomMaskValue)
    return;

  this->UseCustomMaskValue = val;

  this->Modified();

  if (!this->InfoRequested || !this->DataRequested || !this->UseCustomMaskValue)
  {
    return;
  }
  this->DestroyData();
  this->RegenerateGeometry();
}

//----------------------------------------------------------------------------
// Set custom mask value.
//----------------------------------------------------------------------------
void vtkCDIReader::SetCustomMaskValue(double val)
{
  if (val == CustomMaskValue)
    return;

  this->CustomMaskValue = val;

  this->Modified();

  if (!this->InfoRequested || !this->DataRequested || !this->UseCustomMaskValue)
  {
    return;
  }
  this->DestroyData();
  this->RegenerateGeometry();
}

//----------------------------------------------------------------------------
// Get name of indexed Point variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetPointArrayName(int index)
{
  return this->Internals->PointVars[index].Name;
}

//----------------------------------------------------------------------------
// Get name of indexed cell variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetCellArrayName(int index)
{
  return this->Internals->CellVars[index].Name;
}

//----------------------------------------------------------------------------
// Get name of indexed domain variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetDomainArrayName(int index)
{
  return this->Internals->DomainVars[index].c_str();
}

//----------------------------------------------------------------------------
// Set to lat/lon (equidistant cylindrical) projection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetFileName(const char* val)
{
  if (this->FileName.empty() || val == nullptr || strcmp(this->FileName.c_str(), val) != 0)
  {
    if (this->StreamID >= 0)
    {
      streamClose(this->StreamID);
      this->StreamID = -1;
      this->VListID = -1;
    }
    this->Modified();
    if (val == nullptr)
    {
      return;
    }
    this->FileName = val;
    vtkDebugMacro("SetFileName to " << this->FileName << endl);

    this->DestroyData();
    this->RegenerateVariables();
  }
}

//----------------------------------------------------------------------------
//  Set vertical level to be viewed.
//----------------------------------------------------------------------------
void vtkCDIReader::SetVerticalLevel(int level)
{
  if (this->VerticalLevelSelected != level)
  {
    if (level < 0 || level > this->MaximumNVertLevels - 1)
    {
      vtkErrorMacro("Requested inexistent vertical level: "
        << level << ".\nThe level must be the in range [ 0 ; " << this->MaximumNVertLevels - 1
        << " ].");
      return;
    }
    this->VerticalLevelSelected = level;
    this->Modified();
    vtkDebugMacro("Set VerticalLevelSelected to: " << level);
  }
  vtkDebugMacro("InfoRequested?: " << this->InfoRequested);

  if (!this->InfoRequested || !this->DataRequested)
  {
    return;
  }

  for (int var = 0; var < this->NumberOfPointVars; var++)
  {
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro("Loading Point Variable: " << this->Internals->PointVars[var].Name << endl);
      this->LoadPointVarData(var, this->DTime);
    }
  }

  for (int var = 0; var < this->NumberOfCellVars; var++)
  {
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro("Loading Cell Variable: " << this->Internals->CellVars[var].Name << endl);
      this->LoadCellVarData(var, this->DTime);
    }
  }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}

//----------------------------------------------------------------------------
//  Set layer thickness for multilayer view.
//----------------------------------------------------------------------------
void vtkCDIReader::SetLayerThickness(int val)
{
  if (this->LayerThickness != val)
  {
    this->LayerThickness = val;
    this->Modified();
    vtkDebugMacro("SetLayerThickness: LayerThickness set to " << this->LayerThickness << endl);

    if (this->ShowMultilayerView)
    {
      if (!this->InfoRequested || !this->DataRequested)
      {
        return;
      }
      this->DestroyData();
      this->RegenerateGeometry();
    }
  }
}

//----------------------------------------------------------------------------
//  Set layer 0 offset for multilayer view.
//----------------------------------------------------------------------------
void vtkCDIReader::SetLayer0Offset(double val)
{
  if (this->Layer0Offset != val)
  {
    this->Layer0Offset = val;
    this->Modified();
    vtkDebugMacro("SetLayer0Offset: Layer0Offset set to " << this->Layer0Offset << endl);

    if (this->ShowMultilayerView)
    {
      if (!this->InfoRequested || !this->DataRequested)
      {
        return;
      }
      this->DestroyData();
      this->RegenerateGeometry();
    }
  }
}

//----------------------------------------------------------------------------
// Set the projection mode.
//----------------------------------------------------------------------------
void vtkCDIReader::SetProjection(int val)
{
  if (this->ProjectionMode != val)
  {
    if (!isproj(val))
    {
      vtkErrorMacro(
        "SetProjection: Can't set ProjectionMode to '" << val << "'. Not a valid projection.\n");
      return;
    }
    this->ProjectionMode = projection(val);
    this->Modified();
    vtkDebugMacro("SetProjection: ProjectionMode to " << this->ProjectionMode << endl);
    this->ReconstructNew = true;

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set double/float projection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetDoublePrecision(bool val)
{
  if (this->DoublePrecision != val)
  {
    this->DoublePrecision = val;
    this->Modified();
    vtkDebugMacro("DoublePrecision to " << this->DoublePrecision << endl);
    this->ReconstructNew = true;

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set wrapping.
//----------------------------------------------------------------------------
void vtkCDIReader::SetWrapping(bool val)
{
  if (this->WrapOn != val)
  {
    this->WrapOn = val;
    this->Modified();
    vtkDebugMacro("Wrapping set to " << this->WrapOn << endl);
    this->ReconstructNew = true;

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Invert the z-axis of the visualization.
//----------------------------------------------------------------------------
void vtkCDIReader::SetInvertZAxis(bool val)
{
  if (this->InvertZAxis != val)
  {
    this->InvertZAxis = val;
    this->Modified();
    vtkDebugMacro("InvertZAxis to " << this->InvertZAxis << endl);

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set visualization with topography/bathymetrie.
//----------------------------------------------------------------------------
void vtkCDIReader::SetUseMask(bool val)
{
  if (this->UseMask != val)
  {
    this->UseMask = val;
    this->Modified();
    vtkDebugMacro("Set UseMask to " << this->UseMask << endl);

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Add Clon and Clat to output
//----------------------------------------------------------------------------
void vtkCDIReader::SetShowClonClat(bool val)
{
  if (this->ShowClonClat != val)
  {
    this->ShowClonClat = val;
    this->Modified();
    vtkDebugMacro("Set ShowClonClat to " << this->ShowClonClat << endl);

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set visualization with inverted topography/bathymetrie.
//----------------------------------------------------------------------------
void vtkCDIReader::SetInvertMask(bool val)
{
  if (val == this->InvertMask)
  {
    return;
  }
  this->InvertMask = val;
  this->Modified();

  vtkDebugMacro("Set InvertMask to " << this->InvertMask << endl);

  if (!this->InfoRequested || !this->DataRequested)
  {
    return;
  }

  this->DestroyData();
  this->RegenerateGeometry();
}
void vtkCDIReader::SetSkipGrid(bool val)
{
  SkipGrid = val;
}

//----------------------------------------------------------------------------
//  Set view to be multilayered view.
//----------------------------------------------------------------------------
void vtkCDIReader::SetShowMultilayerView(bool val)
{
  if (this->ShowMultilayerView != val)
  {
    this->ShowMultilayerView = val;
    this->Modified();
    vtkDebugMacro("ShowMultilayerView to " << this->ShowMultilayerView << endl);

    if (!this->InfoRequested || !this->DataRequested)
    {
      return;
    }

    this->DestroyData();
    this->RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
//  Print self.
//----------------------------------------------------------------------------
void vtkCDIReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName.c_str() ? this->FileName.c_str() : "nullptr")
     << "\n";
  os << indent << "VariableDimensions: " << this->VariableDimensions << endl;
  os << indent << "AllDimensions: " << this->AllDimensions << endl;
  os << indent << "this->NumberOfPointVars: " << this->NumberOfPointVars << "\n";
  os << indent << "this->NumberOfCellVars: " << this->NumberOfCellVars << "\n";
  os << indent << "this->NumberOfDomainVars: " << this->NumberOfDomainVars << "\n";
  os << indent << "this->MaximumPoints: " << this->MaximumPoints << "\n";
  os << indent << "this->MaximumCells: " << this->MaximumCells << "\n";
  os << indent << "Projection: " << this->ProjectionMode << endl;
  os << indent << "DoublePrecision: " << (this->DoublePrecision ? "ON" : "OFF") << endl;
  os << indent << "Wrapping: " << (this->WrapOn ? "ON" : "OFF") << endl;
  os << indent << "ShowClonClat: " << (this->ShowClonClat ? "ON" : "OFF") << endl;
  os << indent << "ShowMultilayerView: " << (this->ShowMultilayerView ? "ON" : "OFF") << endl;
  os << indent << "InvertZ: " << (this->InvertZAxis ? "ON" : "OFF") << endl;
  os << indent << "UseMask: " << (this->UseMask ? "ON" : "OFF") << endl;
  os << indent << "CustomMaskValue: " << this->CustomMaskValue << endl;
  os << indent << "SkipGrid: " << (this->SkipGrid ? "ON" : "OFF") << endl;
  os << indent << "InvertMask: " << (this->InvertMask ? "ON" : "OFF") << endl;
  os << indent << "VerticalLevel: " << this->VerticalLevelSelected << "\n";
  os << indent << "VerticalLevelRange: " << this->VerticalLevelRange[0] << ","
     << this->VerticalLevelRange[1] << endl;
  os << indent << "LayerThicknessRange: " << this->LayerThicknessRange[0] << ","
     << this->LayerThicknessRange[1] << endl;
  os << indent << "Layer0OffsetRange: " << this->Layer0OffsetRange[0] << ","
     << this->Layer0OffsetRange[1] << endl;
}
