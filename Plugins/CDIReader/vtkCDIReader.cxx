// -*- c++ -*-
/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    vtkCDIReader.cxx
 *
 *  Copyright (c) 2015 Niklas Roeber, DKRZ Hamburg
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
// netCDF data sets with point and cell variables, both 2D and 3D. It allows
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
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTableExtentTranslator.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"
#include "vtk_netcdfcpp.h"

#include "cdi.h"
#include "stdlib.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#define DEBUG 0
#define MAX_VARS 100
#define DEFAULT_LAYER_THICKNESS 50
#define EARTH_RADIUS 6.371229
#define MyPI 3.1415926535897932384
#define DEG2RAD (MyPI / 180.)

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
      this->cellVarIDs[i] = -1;
      //  this->cellVars[i] = NULL;
      //  this->pointVars[i] = NULL;
      this->domainVars[i] = std::string("");
    }
  };
  ~Internal(){};

  int cellVarIDs[MAX_VARS];
  cdiVar_t cellVars[MAX_VARS];
  cdiVar_t pointVars[MAX_VARS];
  string domainVars[MAX_VARS];
};

//----------------------------------------------------------------------------
//  CDI helper functions
//----------------------------------------------------------------------------
void cdi_set_cur(cdiVar_t* cdiVar, int timestep, int level)
{
  cdiVar->timestep = timestep;
  cdiVar->levelID = level;
}

void cdi_get(cdiVar_t* cdiVar, double* buffer, int nlevels)
{
  int nmiss;
  int nrecs = streamInqTimestep(cdiVar->streamID, cdiVar->timestep);
  if (nlevels == 1)
    streamReadVarSlice(cdiVar->streamID, cdiVar->varID, cdiVar->levelID, buffer, &nmiss);
  else
    streamReadVar(cdiVar->streamID, cdiVar->varID, buffer, &nmiss);
  // dummy calculation
  nmiss += nrecs;
}

//----------------------------------------------------------------------------
// Macro to check malloc didn't return an error
//----------------------------------------------------------------------------
#define CHECK_MALLOC(ptr)                                                                          \
  if (ptr == NULL)                                                                                 \
  {                                                                                                \
    vtkErrorMacro(<< "malloc failed!" << endl);                                                    \
    return (0);                                                                                    \
  }

//----------------------------------------------------------------------------
//  Macro to check if the named NetCDF dimension exists
//----------------------------------------------------------------------------
#define CHECK_DIM(ncFile, name)                                                                    \
  if (!isNcDim(ncFile, name))                                                                      \
  {                                                                                                \
    vtkErrorMacro(<< "Cannot find dimension: " << name << endl);                                   \
    return 0;                                                                                      \
  }

//----------------------------------------------------------------------------
// Check if there is a NetCDF dimension by that name
//----------------------------------------------------------------------------
static bool isNcDim(NcFile* ncFile, NcToken name)
{
  int num_dims = ncFile->num_dims();
  // cerr << "looking for: " << name << endl;
  for (int i = 0; i < num_dims; i++)
  {
    NcDim* ncDim = ncFile->get_dim(i);
    // cerr << "checking " << ncDim->name() << endl;
    if ((strcmp(ncDim->name(), name)) == 0)
      // we have a match, so return
      return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
//  Function to convert cartesian coordinates to spherical, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------
static int CartesianToSpherical(
  double x, double y, double z, double* rho, double* phi, double* theta)
{
  double trho, ttheta, tphi;

  trho = sqrt((x * x) + (y * y) + (z * z));
  ttheta = atan2(y, x);
  tphi = acos(z / (trho));
  if (vtkMath::IsNan(trho) || vtkMath::IsNan(ttheta) || vtkMath::IsNan(tphi))
    return -1;

  *rho = trho;
  *theta = ttheta;
  *phi = tphi;

  return 0;
}

//----------------------------------------------------------------------------
//  Function to convert spherical coordinates to cartesian, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------
static int SphericalToCartesian(
  double rho, double phi, double theta, double* x, double* y, double* z)
{
  double tx, ty, tz;

  tx = rho * sin(phi) * cos(theta);
  ty = rho * sin(phi) * sin(theta);
  tz = rho * cos(phi);
  if (vtkMath::IsNan(tx) || vtkMath::IsNan(ty) || vtkMath::IsNan(tz))
    return -1;

  *x = tx;
  *y = ty;
  *z = tz;

  return 0;
}

//----------------------------------------------------------------------------
//  Function to convert lon/lat coordinates to cartesian
//----------------------------------------------------------------------------
static int LLtoXYZ(
  double lon, double lat, double* x, double* y, double* z, bool ProjectLatLon, bool ProjectCassini)
{
  double tx, ty, tz;
  static double const pi = 3.14159265358979323846;

  if (ProjectLatLon)
  {
    tx = (lon * cos(0.0));
    ty = lat;
    tz = 0.0;
  }
  else if (ProjectCassini)
  {
    tx = 50.0 * asin(cos(lat) * sin(lon));
    ty = 50.0 * atan2(sin(lat), (cos(lat) * cos(lon)));
    tz = 0.0;
  }
  else
  {
    lat += pi * 0.5;
    tx = sin(lat) * cos(lon) * 200.0;
    ty = sin(lat) * sin(lon) * 200.0;
    tz = cos(lat) * 200.0;
  }

  if (vtkMath::IsNan(tx) || vtkMath::IsNan(ty) || vtkMath::IsNan(tz))
    return -1;

  *x = tx;
  *y = ty;
  *z = tz;

  return 1;
}

// Strip leading and trailing punctuation
// http://answers.yahoo.com/question/index?qid=20081226034047AAzf8lj
void strip(string& s)
{
  string::iterator i = s.begin();
  while (ispunct(*i))
    s.erase(i);
  string::reverse_iterator j = s.rbegin();
  while (ispunct(*j))
  {
    s.resize(s.length() - 1);
    j = s.rbegin();
  }
}

string convertInt(int number)
{
  stringstream ss;
  ss << number;
  return ss.str();
}

vtkStandardNewMacro(vtkCDIReader);

//----------------------------------------------------------------------------
// Constructor for vtkCDIReader
//----------------------------------------------------------------------------
vtkCDIReader::vtkCDIReader()
{
  this->Internals = new vtkCDIReader::Internal;
  this->streamID = -1;
  this->vlistID = -1;
  this->CellMask = 0;
  this->LoadingDimensions = vtkSmartPointer<vtkIntArray>::New();
  this->VariableDimensions = vtkStringArray::New();
  this->AllDimensions = vtkStringArray::New();
  this->AllVariableArrayNames = vtkSmartPointer<vtkStringArray>::New();

  // Debugging
  if (DEBUG)
    this->DebugOn();
  vtkDebugMacro(<< "Starting to create vtkCDIReader..." << endl);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->InfoRequested = false;
  this->DataRequested = false;
  this->haveDomainData = false;
  this->SetDefaults();

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

  vtkDebugMacro(<< "MAX_VARS:" << MAX_VARS << endl);
  vtkDebugMacro(<< "Created vtkCDIReader" << endl);
}

//----------------------------------------------------------------------------
//  Destroys data stored for variables, points, and cells, but
//  doesn't destroy the list of variables or toplevel cell/pointVarDataArray.
//----------------------------------------------------------------------------
void vtkCDIReader::DestroyData()
{
  vtkDebugMacro(<< "DestroyData..." << endl);

  vtkDebugMacro(<< "Destructing cell var data..." << endl);
  if (this->CellVarDataArray)
    for (int i = 0; i < this->NumberOfCellVars; i++)
      if (this->CellVarDataArray[i] != NULL)
      {
        this->CellVarDataArray[i]->Delete();
        this->CellVarDataArray[i] = NULL;
      }

  vtkDebugMacro(<< "Destructing point var array..." << endl);
  if (this->PointVarDataArray)
    for (int i = 0; i < this->NumberOfPointVars; i++)
      if (this->PointVarDataArray[i] != NULL)
      {
        this->PointVarDataArray[i]->Delete();
        this->PointVarDataArray[i] = NULL;
      }

  if (this->DomainVarDataArray)
    for (int i = 0; i < this->NumberOfDomainVars; i++)
      if (this->DomainVarDataArray[i] != NULL)
      {
        this->DomainVarDataArray[i]->Delete();
        this->DomainVarDataArray[i] = NULL;
      }

  if (this->reconstruct_new)
  {
    if (this->PointVarData)
    {
      delete[] this->PointVarData;
      this->PointVarData = NULL;
    }
    if (this->CellMap)
    {
      free(this->CellMap);
      this->CellMap = NULL;
    }
    if (this->PointMap)
    {
      free(this->PointMap);
      this->PointMap = NULL;
    }
    if (this->MaximumLevelPoint)
    {
      free(this->MaximumLevelPoint);
      this->MaximumLevelPoint = NULL;
    }
  }
}

//----------------------------------------------------------------------------
// Destructor for vtkCDIReader
//----------------------------------------------------------------------------
vtkCDIReader::~vtkCDIReader()
{
  vtkDebugMacro(<< "Destructing vtkCDIReader..." << endl);

  this->SetFileName(NULL);

  if (this->streamID >= 0)
  {
    streamClose(this->streamID);
    this->streamID = -1;
  }

  this->DestroyData();

  if (this->CellVarDataArray)
  {
    delete[] this->CellVarDataArray;
    this->CellVarDataArray = NULL;
  }

  if (this->PointVarDataArray)
  {
    delete[] this->PointVarDataArray;
    this->PointVarDataArray = NULL;
  }

  if (this->DomainVarDataArray)
  {
    delete[] this->DomainVarDataArray;
    this->DomainVarDataArray = NULL;
  }

  vtkDebugMacro(<< "Destructing other stuff..." << endl);
  if (this->PointDataArraySelection)
  {
    this->PointDataArraySelection->Delete();
    this->PointDataArraySelection = NULL;
  }
  if (this->CellDataArraySelection)
  {
    this->CellDataArraySelection->Delete();
    this->CellDataArraySelection = NULL;
  }
  if (this->DomainDataArraySelection)
  {
    this->DomainDataArraySelection->Delete();
    this->DomainDataArraySelection = NULL;
  }
  if (this->SelectionObserver)
  {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
  }
  if (this->TimeSteps)
  {
    delete[] this->TimeSteps;
    this->TimeSteps = NULL;
  }

  this->VariableDimensions->Delete();
  this->AllDimensions->Delete();
  delete this->Internals;
  vtkDebugMacro(<< "Destructed vtkCDIReader" << endl);
}

//----------------------------------------------------------------------------
// Verify that the file exists, get dimension sizes and variables
//----------------------------------------------------------------------------
int vtkCDIReader::RequestInformation(
  vtkInformation* reqInfo, vtkInformationVector** inVector, vtkInformationVector* outVector)
{
  vtkDebugMacro(<< "In vtkCDIReader::RequestInformation" << endl);
  if (!this->Superclass::RequestInformation(reqInfo, inVector, outVector))
    return 0;

  if (!this->FileName)
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  vtkDebugMacro(<< "In vtkCDIReader::RequestInformation read filename okay" << endl);
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  if (!this->InfoRequested)
  {
    this->InfoRequested = true;
    vtkDebugMacro(<< "FileName: " << this->FileName << endl);

    this->streamID = streamOpenRead(this->FileName);
    if (this->streamID < 0)
    {
      vtkDebugMacro(<< "Couldn't open file: " << cdiStringError(this->streamID) << endl);
      vtkErrorMacro(<< "Couldn't open file: " << cdiStringError(this->streamID) << endl);
      return 0;
    }

    vtkDebugMacro(<< "In vtkCDIReader::RequestInformation read file okay" << endl);
    this->vlistID = streamInqVlist(this->streamID);

    int nvars = vlistNvars(this->vlistID);
    char varname[CDI_MAX_NAME];
    for (int varID = 0; varID < nvars; ++varID)
      vlistInqVarName(this->vlistID, varID, varname);

    if (!GetDims())
      return (0);

    vtkDebugMacro(<< "In vtkCDIReader::RequestInformation setting VerticalLevelRange" << endl);
    this->VerticalLevelRange[0] = 0;
    this->VerticalLevelRange[1] = this->MaximumNVertLevels - 1;

    if (!BuildVarArrays())
      return 0;

    if (this->PointVarDataArray)
      delete[] this->PointVarDataArray;

    this->PointVarDataArray = new vtkDoubleArray*[this->NumberOfPointVars];
    for (int i = 0; i < this->NumberOfPointVars; i++)
      this->PointVarDataArray[i] = NULL;

    if (this->CellVarDataArray)
      delete[] this->CellVarDataArray;

    this->CellVarDataArray = new vtkDoubleArray*[this->NumberOfCellVars];
    for (int i = 0; i < this->NumberOfCellVars; i++)
      this->CellVarDataArray[i] = NULL;

    if (this->DomainVarDataArray)
      delete[] this->DomainVarDataArray;

    this->DomainVarDataArray = new vtkDoubleArray*[this->NumberOfDomainVars];
    for (int i = 0; i < this->NumberOfDomainVars; i++)
      this->DomainVarDataArray[i] = NULL;

    this->DisableAllPointArrays();
    this->DisableAllCellArrays();
    this->DisableAllDomainArrays();

    if (this->TimeSteps != NULL)
    {
      delete[] this->TimeSteps;
      this->TimeSteps = NULL;
    }

    this->TimeSteps = new double[this->NumberOfTimeSteps];
    for (int step = 0; step < this->NumberOfTimeSteps; step++)
      this->TimeSteps[step] = (double)step;

    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumberOfTimeSteps);

    double tRange[2];
    tRange[0] = this->TimeSteps[0];
    tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Default method: Data is read into a vtkUnstructuredGrid
//----------------------------------------------------------------------------
int vtkCDIReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  vtkDebugMacro(<< "In vtkCDIReader::RequestData" << endl);
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->DataRequested)
    this->DestroyData();

  if (!this->ReadAndOutputGrid(true))
    return 0;

  double requestedTimeStep(0);
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

  vtkDebugMacro(<< "Num Time steps requested: " << numRequestedTimeSteps << endl);
  this->DTime = requestedTimeStep;
  vtkDebugMacro(<< "this->DTime: " << this->DTime << endl);
  double dTimeTemp = this->DTime;
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTimeTemp);
  vtkDebugMacro(<< "dTimeTemp: " << dTimeTemp << endl);
  this->DTime = dTimeTemp;

  for (int var = 0; var < this->NumberOfPointVars; var++)
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Point Variable: " << var << endl);
      if (!this->LoadPointVarData(var, this->DTime))
        return 0;

      output->GetPointData()->AddArray(this->PointVarDataArray[var]);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Cell Variable: " << this->Internals->cellVars[var].name << endl);
      this->LoadCellVarData(var, this->DTime);
      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
    }

  for (int var = 0; var < this->NumberOfDomainVars; var++)
    if (this->DomainDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Domain Variable: " << this->Internals->domainVars[var].c_str()
                    << endl);
      this->LoadDomainVarData(var);
      output->GetFieldData()->AddArray(this->DomainVarDataArray[var]);
    }

  if (this->buildDomainArrays)
    this->buildDomainArrays = this->BuildDomainCellVars();

  this->DataRequested = true;
  vtkDebugMacro(<< "Returning from RequestData" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Regenrate and reread the data variables available
//----------------------------------------------------------------------------
int vtkCDIReader::RegenerateVariables()
{
  this->NumberOfPointVars = 0;
  this->NumberOfCellVars = 0;
  this->NumberOfDomainVars = 0;

  if (!GetDims())
    return (0);

  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = this->MaximumNVertLevels - 1;

  if (!BuildVarArrays())
    return 0;

  // Allocate the ParaView data arrays which will hold the variables
  if (this->PointVarDataArray)
    delete[] this->PointVarDataArray;

  this->PointVarDataArray = new vtkDoubleArray*[this->NumberOfPointVars];
  for (int i = 0; i < this->NumberOfPointVars; i++)
    this->PointVarDataArray[i] = NULL;

  if (this->CellVarDataArray)
    delete[] this->CellVarDataArray;

  this->CellVarDataArray = new vtkDoubleArray*[this->NumberOfCellVars];
  for (int i = 0; i < this->NumberOfCellVars; i++)
    this->CellVarDataArray[i] = NULL;

  if (this->DomainVarDataArray)
    delete[] this->DomainVarDataArray;

  this->DomainVarDataArray = new vtkDoubleArray*[this->NumberOfDomainVars];
  for (int i = 0; i < this->NumberOfDomainVars; i++)
    this->DomainVarDataArray[i] = NULL;

  // Start with no data loaded into ParaView
  this->DisableAllPointArrays();
  this->DisableAllCellArrays();
  this->DisableAllDomainArrays();

  return 1;
}

//----------------------------------------------------------------------------
// Set defaults for various parameters and initialize some variables
//----------------------------------------------------------------------------
void vtkCDIReader::SetDefaults()
{
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = 1;
  this->VerticalLevelSelected = 0;

  this->LayerThicknessRange[0] = 0;
  this->LayerThicknessRange[1] = 100;
  this->LayerThickness = 50;

  // this is hard coded for now but will change when data generation gets more mature
  this->performance_data_file = "timer.atmo.";
  this->domain_var_name = "cell_owner";
  this->domain_dimension = "domains";
  this->haveDomainVariable = false;
  this->haveDomainData = false;

  this->dimensionSelection = 0;
  this->InvertZAxis = false;
  this->ProjectLatLon = false;
  this->ProjectCassini = false;
  this->ShowMultilayerView = false;
  this->reconstruct_new = false;
  this->CellDataSelected = 0;
  this->PointDataSelected = 0;
  this->gotMask = false;

  this->grid_reconstructed = false;
  this->RemoveMissingValues = true;
  this->MissingValue = 0.0;
  this->masking_value = 0.0;
  this->invertedTopography = false;
  this->IncludeTopography = false;

  this->PointX = NULL;
  this->PointY = NULL;
  this->PointZ = NULL;
  this->OrigConnections = NULL;
  this->ModConnections = NULL;
  this->CellMap = NULL;
  this->PointMap = NULL;
  this->MaximumLevelPoint = NULL;

  this->FileName = NULL;
  this->DTime = 0;
  this->CellVarDataArray = NULL;
  this->PointVarDataArray = NULL;
  this->DomainVarDataArray = NULL;
  this->PointVarData = NULL;
  this->TimeSteps = NULL;
  this->buildDomainArrays = false;

  this->DomainMask = (int*)malloc(MAX_VARS * sizeof(int));
  for (int i = 0; i < MAX_VARS; i++)
    this->DomainMask[i] = 0;
}

//----------------------------------------------------------------------------
// Get dimensions of key NetCDF variables
//----------------------------------------------------------------------------
int vtkCDIReader::GetDims()
{
  int vlistID_l = this->vlistID;
  this->gridID = -1;
  this->zaxisID = -1;
  this->surfID = -1;

  int ngrids = vlistNgrids(vlistID_l);
  for (int i = 0; i < ngrids; ++i)
  {
    int gridID_l = vlistGrid(vlistID_l, i);
    int nv = gridInqNvertex(gridID_l);

    if (nv != 3 && nv != 4)
      continue;
    if (gridInqType(gridID_l) == GRID_UNSTRUCTURED)
    {
      this->gridID = gridID_l;
      break;
    }
  }

  if (this->gridID == -1)
    vtkErrorMacro(<< "Horizontal grid not found!" << endl);

  int nzaxis = vlistNzaxis(vlistID_l);
  for (int i = 0; i < nzaxis; ++i)
  {
    int zaxisID_l = vlistZaxis(vlistID_l, i);

    if (zaxisInqSize(zaxisID_l) == 1 && zaxisInqType(zaxisID_l) == ZAXIS_SURFACE)
    {
      this->surfID = zaxisID_l;
      this->zaxisID = zaxisID_l;
      break;
    }
  }

  for (int i = 0; i < nzaxis; ++i)
  {
    int zaxisID_l = vlistZaxis(vlistID_l, i);
    if (zaxisInqSize(zaxisID_l) > 1)
    {
      this->zaxisID = zaxisID_l;
      break;
    }
  }

  if (this->zaxisID == -1)
    vtkErrorMacro(<< "Vertical grid not found!" << endl);

  if (this->dimensionSelection > 0)
  {
    int zaxisID_l = vlistZaxis(vlistID_l, this->dimensionSelection);
    this->zaxisID = zaxisID_l;
  }

  if (this->gridID != -1)
    this->NumberOfCells = gridInqSize(this->gridID);

  if (this->gridID != -1)
    this->NumberOfPoints = gridInqSize(this->gridID);

  if (this->gridID != -1)
    this->PointsPerCell = gridInqNvertex(this->gridID);

  int ntsteps = vlistNtsteps(this->vlistID);
  if (ntsteps > 0)
    this->NumberOfTimeSteps = ntsteps;
  else
    this->NumberOfTimeSteps = 1;

  this->MaximumNVertLevels = 1;
  if (this->zaxisID != -1)
    this->MaximumNVertLevels = zaxisInqSize(this->zaxisID);

  this->FillVariableDimensions();
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

  int numVars = vlistNvars(this->vlistID);
  for (int i = 0; i < numVars; i++)
  {
    int varID = i;
    cdiVar_t aVar;

    aVar.streamID = streamID;
    aVar.varID = varID;
    aVar.gridID = vlistInqVarGrid(vlistID, varID);
    aVar.zaxisID = vlistInqVarZaxis(vlistID, varID);
    aVar.gridsize = gridInqSize(aVar.gridID);
    aVar.nlevel = zaxisInqSize(aVar.zaxisID);
    aVar.type = 0;
    aVar.const_time = 0;

    if (vlistInqVarTsteptype(vlistID, varID) == TSTEP_CONSTANT)
      aVar.const_time = 1;
    if (aVar.zaxisID != this->zaxisID && aVar.zaxisID != this->surfID)
      continue;
    if (gridInqType(aVar.gridID) != GRID_UNSTRUCTURED)
      continue;

    vlistInqVarName(vlistID, varID, aVar.name);
    aVar.type = 2;
    if (aVar.nlevel > 1)
      aVar.type = 3;
    int varType = aVar.type;

    // check for dim 2 being cell or point
    bool isCellData = false;
    bool isPointData = false;
    if (aVar.gridsize == this->NumberOfCells)
      isCellData = true;
    else if (aVar.gridsize < this->NumberOfCells)
    {
      isPointData = true;
      this->NumberOfPoints = aVar.gridsize;
    }
    else
      continue;

    // 3D variables ...
    if (varType == 3)
    {
      if (isCellData)
      {
        cellVarIndex++;
        if (cellVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro(<< "Exceeded number of cell vars." << endl);
          return (0);
        }
        this->Internals->cellVars[cellVarIndex] = aVar;
        vtkDebugMacro(<< "Adding var " << aVar.name << " to cellVars" << endl);
      }
      else if (isPointData)
      {
        pointVarIndex++;
        if (pointVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro(<< "Exceeded number of point vars." << endl);
          return (0);
        }
        this->Internals->pointVars[pointVarIndex] = aVar;
        vtkDebugMacro(<< "Adding var " << aVar.name << " to pointVars" << endl);
      }
    }
    // 2D variables
    else if (varType == 2)
    {
      vtkDebugMacro(<< "check for " << aVar.name << "." << endl);
      if (isCellData)
      {
        cellVarIndex++;
        if (cellVarIndex > MAX_VARS - 1)
        {
          vtkDebugMacro(<< "Exceeded number of cell vars." << endl);
          return (0);
        }
        this->Internals->cellVars[cellVarIndex] = aVar;
        vtkDebugMacro(<< "Adding var " << aVar.name << " to cellVars" << endl);
      }
      else if (isPointData)
      {
        pointVarIndex++;
        if (pointVarIndex > MAX_VARS - 1)
        {
          vtkErrorMacro(<< "Exceeded number of point vars." << endl);
          return (0);
        }
        this->Internals->pointVars[pointVarIndex] = aVar;
        vtkDebugMacro(<< "Adding var " << aVar.name << " to pointVars" << endl);
      }
    }
  }

  // check if domain var is defined and how many domains are available
  for (int var = 0; var < pointVarIndex + 1; var++)
    if (!strcmp(Internals->pointVars[var].name, this->domain_var_name.c_str()))
      haveDomainVariable = true;
  for (int var = 0; var < cellVarIndex + 1; var++)
    if (!strcmp(Internals->cellVars[var].name, this->domain_var_name.c_str()))
      haveDomainVariable = true;

  // prepare data structure and read in names
  string filename = performance_data_file + "0000";
  ifstream file(filename.c_str());
  if (file)
    haveDomainData = true;

  if (SupportDomainData())
  {
    vtkDebugMacro(<< "Found Domain Data and loading it." << endl);
    this->buildDomainArrays = true;

    NcFile* pnf = new NcFile(this->FileName);
    CHECK_DIM(pnf, this->domain_dimension.c_str());
    NcDim* nDomains = pnf->get_dim(this->domain_dimension.c_str());
    this->NumberOfDomains = nDomains->size();
    vtkDebugMacro(<< "We have a total of " << this->NumberOfDomains << " Domains." << endl);

    string str, word;
    getline(file, str); // discard the first entry
    int line = 0;
    while (getline(file, str))
    {
      vector<string> wordVec;
      vector<string>::iterator i;
      stringstream ss(str);
      while (ss.good())
      {
        ss >> word;
        strip(word);
        wordVec.push_back(word);
      }
      line++;
      if ((wordVec.at(0) == "00") && !(wordVec.at(1) == "L"))
      {
        domainVarIndex++;
        DomainMask[domainVarIndex] = line;
        this->Internals->domainVars[domainVarIndex] = std::string(wordVec.at(1));
      }
    }
  }

  this->NumberOfPointVars = pointVarIndex + 1;
  this->NumberOfCellVars = cellVarIndex + 1;
  this->NumberOfDomainVars = domainVarIndex + 1;
  return (1);
}

//----------------------------------------------------------------------------
// Build the selection Arrays for points and cells in the GUI.
//----------------------------------------------------------------------------
int vtkCDIReader::BuildVarArrays()
{
  vtkDebugMacro(<< "In vtkCDIReader::BuildVarArrays" << endl);

  if (!GetVars())
    return 0;
  vtkDebugMacro(<< "NumberOfCellVars: " << this->NumberOfCellVars
                << " NumberOfPointVars: " << this->NumberOfPointVars << endl);

  if (this->NumberOfCellVars == 0)
    vtkErrorMacro(<< "No cell variables found!" << endl);

  for (int var = 0; var < this->NumberOfPointVars; var++)
  {
    this->PointDataArraySelection->EnableArray((const char*)(this->Internals->pointVars[var].name));
    vtkDebugMacro(<< "Adding point var: " << this->Internals->pointVars[var].name << endl);
  }

  for (int var = 0; var < this->NumberOfCellVars; var++)
  {
    vtkDebugMacro(<< "Adding cell var: " << this->Internals->cellVars[var].name << endl);
    this->CellDataArraySelection->EnableArray((const char*)(this->Internals->cellVars[var].name));
  }

  for (int var = 0; var < this->NumberOfDomainVars; var++)
  {
    vtkDebugMacro(<< "Adding domain var: " << this->Internals->domainVars[var].c_str() << endl);
    this->DomainDataArraySelection->EnableArray(
      (const char*)(this->Internals->domainVars[var].c_str()));
  }

  vtkDebugMacro(<< "Leaving vtkCDIReader::BuildVarArrays" << endl);
  return (1);
}

//----------------------------------------------------------------------------
//  Read the data from the ncfile, allocate the geometry and create the
//  vtk data structures for points and cells.
//----------------------------------------------------------------------------
int vtkCDIReader::ReadAndOutputGrid(bool init)
{
  vtkDebugMacro(<< "In vtkCDIReader::ReadAndOutputGrid" << endl);

  if (!ProjectLatLon && !ProjectCassini)
  {
    if (!AllocSphereGeometry())
      return 0;
  }
  else
  // project equidistant cylindrical
  {
    if (!AllocLatLonGeometry())
      return 0;

    if (ProjectLatLon && !EliminateXWrap())
      return 0;
    if (ProjectCassini && !EliminateYWrap())
      return 0;
  }

  OutputPoints(init);
  OutputCells(init);

  // Allocate the data arrays which will hold the NetCDF var data
  vtkDebugMacro(<< "pointVarData: Alloc " << this->MaximumPoints << " doubles" << endl);
  if (this->PointVarData)
    delete[] this->PointVarData;

  this->PointVarData = new double[this->MaximumPoints];
  vtkDebugMacro(<< "Leaving vtkCDIReader::ReadAndOutputGrid" << endl);

  return (1);
}

//----------------------------------------------------------------------------
// Mirrors the triangle mesh in z direction
//----------------------------------------------------------------------------
int vtkCDIReader::MirrorMesh()
{
  for (int i = 0; i < this->NumberOfPoints; i++)
    this->PointZ[i] = (this->PointZ[i] * (-1.0));

  return 1;
}

//----------------------------------------------------------------------------
// Routines for sorting and efficient removal of duplicates
// (c) and thanks to Moritz Hanke (DKRZ)
//----------------------------------------------------------------------------
int compare_point_with_index(const void* a, const void* b)
{
  const struct point_with_index* a_ = (struct point_with_index*)a;
  const struct point_with_index* b_ = (struct point_with_index*)b;
  double threshold = 1e-22;

  int lon_diff = fabs(a_->p.lon - b_->p.lon) > threshold;
  int lat_diff = fabs(a_->p.lat - b_->p.lat) > threshold;

  if (lon_diff)
  {
    if (a_->p.lon > b_->p.lon)
      return -1;
    else
      return 1;
  }
  else if (lat_diff)
  {
    if (a_->p.lat > b_->p.lat)
      return -1;
    else
      return 1;
  }
  else
    return 0;
}

void vtkCDIReader::Remove_Duplicates(
  double* PointLon, double* PointLat, int temp_nbr_vertices, int* vertexID, int* nbr_cells)
{
  struct point_with_index* sort_array =
    (point_with_index*)malloc(temp_nbr_vertices * sizeof(point_with_index));

  for (int i = 0; i < temp_nbr_vertices; ++i)
  {
    double curr_lon, curr_lat;
    double threshold = (MyPI / 2.0) - 1e-4;
    curr_lon = ((double*)PointLon)[i];
    curr_lat = ((double*)PointLat)[i];

    while (curr_lon < 0.0)
      curr_lon += 2 * MyPI;
    while (curr_lon >= MyPI)
      curr_lon -= 2 * MyPI;

    if (curr_lat > threshold)
      curr_lon = 0.0;
    else if (curr_lat < (-1.0 * threshold))
      curr_lon = 0.0;

    sort_array[i].p.lon = curr_lon;
    sort_array[i].p.lat = curr_lat;

    sort_array[i].i = i;
  }

  qsort(sort_array, temp_nbr_vertices, sizeof(*sort_array), compare_point_with_index);
  vertexID[sort_array[0].i] = 1;

  int last_unique_idx = sort_array[0].i;
  for (int i = 1; i < temp_nbr_vertices; ++i)
  {
    if (compare_point_with_index(sort_array + i - 1, sort_array + i))
    {
      vertexID[sort_array[i].i] = 1;
      last_unique_idx = sort_array[i].i;
    }
    else
      vertexID[sort_array[i].i] = -last_unique_idx;
  }
  free(sort_array);

  int new_nbr_vertices = 0;
  for (int i = 0; i < temp_nbr_vertices; ++i)
  {
    if (vertexID[i] == 1)
    {
      ((double*)PointLon)[new_nbr_vertices] = ((double*)PointLon)[i];
      ((double*)PointLat)[new_nbr_vertices] = ((double*)PointLat)[i];
      vertexID[i] = new_nbr_vertices;
      new_nbr_vertices++;
    }
  }

  for (int i = 0; i < temp_nbr_vertices; ++i)
    if (vertexID[i] <= 0)
      vertexID[i] = vertexID[-vertexID[i]];

  nbr_cells[0] = temp_nbr_vertices;
  nbr_cells[1] = new_nbr_vertices;
}

//----------------------------------------------------------------------------
// Construct grid geometry
//----------------------------------------------------------------------------
int vtkCDIReader::ConstructGridGeometry()
{
  vtkDebugMacro(<< "Starting grid reconstruction ..." << endl);
  this->clon_vertices =
    (double*)malloc((this->NumberOfCells) * this->PointsPerCell * sizeof(double));
  this->clat_vertices =
    (double*)malloc((this->NumberOfCells) * this->PointsPerCell * sizeof(double));
  this->depth_var = (double*)malloc(this->MaximumNVertLevels * sizeof(double));

  CHECK_MALLOC(this->clon_vertices);
  CHECK_MALLOC(this->clat_vertices);
  CHECK_MALLOC(this->depth_var);

  gridInqXbounds(this->gridID, this->clon_vertices);
  gridInqYbounds(this->gridID, this->clat_vertices);
  zaxisInqLevels(this->zaxisID, this->depth_var);

  char units[CDI_MAX_NAME];
  int points = (this->NumberOfCells) * this->PointsPerCell;
  int* vertexID = (int*)malloc(points * sizeof(int));
  int* new_cells = (int*)malloc(2 * sizeof(int));

  gridInqXunits(gridID, units);
  if (strncmp(units, "degree", 6) == 0)
    for (int i = 0; i < points; i++)
      this->clon_vertices[i] *= DEG2RAD;
  gridInqYunits(gridID, units);
  if (strncmp(units, "degree", 6) == 0)
    for (int i = 0; i < points; i++)
      this->clat_vertices[i] *= DEG2RAD;
  vtkDebugMacro(<< "Read in geometry data ..." << endl);

  // check for duplicates in the point list and update the vertex list
  this->Remove_Duplicates(this->clon_vertices, this->clat_vertices, points, vertexID, new_cells);
  this->NumberOfCells = floor(new_cells[0] / 3.0);
  this->NumberOfPoints = new_cells[1];
  vtkDebugMacro(<< "Sorting done ..." << endl);

  if (!ProjectLatLon && !ProjectCassini)
  {
    this->PointX = (double*)malloc(this->NumberOfPoints * sizeof(double));
    this->PointY = (double*)malloc(this->NumberOfPoints * sizeof(double));
    this->PointZ = (double*)malloc(this->NumberOfPoints * sizeof(double));
  }
  else
  {
    this->PointX = (double*)malloc(this->ModNumPoints * sizeof(double));
    this->PointY = (double*)malloc(this->ModNumPoints * sizeof(double));
    this->PointZ = (double*)malloc(this->ModNumPoints * sizeof(double));
  }
  CHECK_MALLOC(this->PointX);
  CHECK_MALLOC(this->PointY);
  CHECK_MALLOC(this->PointZ);

  // now get the individual coordinates out of the clon/clat vertices
  for (int i = 0; i < this->NumberOfPoints; i++)
    LLtoXYZ(this->clon_vertices[i], this->clat_vertices[i], &PointX[i], &PointY[i], &PointZ[i],
      ProjectLatLon, ProjectCassini);
  vtkDebugMacro(<< "Projection done ..." << endl);

  // create final triangle list
  this->OrigConnections = (int*)malloc(this->NumberOfCells * this->PointsPerCell * sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  vtkDebugMacro(<< "Made Connections ..." << endl);

  // use the vertexID as triangle list
  for (int i = 0; i < (this->NumberOfCells * this->PointsPerCell); i++)
    this->OrigConnections[i] = vertexID[i];

  // mirror the mesh if needed
  if (!ProjectLatLon && !ProjectCassini)
    this->MirrorMesh();
  grid_reconstructed = true;
  reconstruct_new = false;
  free(vertexID);

  vtkDebugMacro(<< "Grid Reconstruction complete..." << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Allocate into sphere view of geometry
//----------------------------------------------------------------------------
int vtkCDIReader::AllocSphereGeometry()
{
  vtkDebugMacro(<< "In AllocSphereGeometry..." << endl);

  if (!grid_reconstructed || this->reconstruct_new)
    this->ConstructGridGeometry();

  this->CurrentExtraPoint = this->NumberOfPoints;
  this->CurrentExtraCell = this->NumberOfCells;

  if (this->ShowMultilayerView)
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    vtkDebugMacro(<< "alloc sphere: multilayer: setting MaximumCells to " << this->MaximumCells);
    this->MaximumPoints = this->CurrentExtraPoint * (this->MaximumNVertLevels + 1);
    vtkDebugMacro(<< "alloc sphere: multilayer: setting MaximumPoints to " << this->MaximumPoints);
  }
  else
  {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro(<< "alloc sphere: singlelayer: setting MaximumPoints to " << this->MaximumPoints);
  }

  CheckForMaskData();
  vtkDebugMacro(<< "Leaving AllocSphereGeometry...");
  return 1;
}

//----------------------------------------------------------------------------
// Allocate the lat/lon or Cassini projection of geometry.
//----------------------------------------------------------------------------
int vtkCDIReader::AllocLatLonGeometry()
{
  const float BLOATFACTOR = .3;
  this->ModNumPoints = (int)floor(this->NumberOfPoints * (1.0 + BLOATFACTOR));
  this->ModNumCells = (int)floor(this->NumberOfCells * (1.0 + BLOATFACTOR)) + 1;

  if (!grid_reconstructed || this->reconstruct_new)
    this->ConstructGridGeometry();

  this->ModConnections = (int*)malloc(this->ModNumCells * this->PointsPerCell * sizeof(int));
  CHECK_MALLOC(this->ModConnections);

  this->PointMap = (int*)malloc((int)floor(this->NumberOfPoints * BLOATFACTOR) * sizeof(int));
  this->CellMap = (int*)malloc((int)floor(this->NumberOfCells * BLOATFACTOR) * sizeof(int));
  CHECK_MALLOC(this->PointMap);
  CHECK_MALLOC(this->CellMap);

  this->CurrentExtraPoint = this->NumberOfPoints;
  this->CurrentExtraCell = this->NumberOfCells;

  if (ShowMultilayerView)
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint * (this->MaximumNVertLevels + 1);
    vtkDebugMacro(<< "alloc latlon: multilayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }
  else
  {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro(<< "alloc latlon: singlelayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }

  CheckForMaskData();
  vtkDebugMacro(<< "Leaving AllocLatLonGeometry..." << endl);
  return 1;
}

//----------------------------------------------------------------------------
//  Read in Land/Sea Mask wet_c to mask out the land surface in ocean
//  simulations.
//----------------------------------------------------------------------------
int vtkCDIReader::CheckForMaskData()
{
  int numVars = vlistNvars(this->vlistID);
  this->gotMask = false;
  int mask_pos = 0;

  for (int i = 0; i < numVars; i++)
    if (!strcmp(this->Internals->cellVars[i].name, "wet_c"))
    {
      this->gotMask = true;
      mask_pos = i;
    }

  if (this->gotMask)
  {
    cdiVar_t* cdiVar = &(this->Internals->cellVars[mask_pos]);
    if (ShowMultilayerView)
    {
      this->CellMask = (int*)malloc(this->MaximumCells * sizeof(int));
      double* dataTmpMask = (double*)malloc(this->MaximumCells * sizeof(double));
      CHECK_MALLOC(this->CellMask);
      CHECK_MALLOC(dataTmpMask);

      cdi_set_cur(cdiVar, 0, 0);
      cdi_get(cdiVar, dataTmpMask, this->MaximumNVertLevels);
      // readjust the data
      for (int j = 0; j < this->NumberOfCells; j++)
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          this->CellMask[i + levelNum] = (int)(dataTmpMask[j + (levelNum * this->NumberOfCells)]);
        }
      free(dataTmpMask);
      vtkDebugMacro(<< "Got data for land/sea mask (3D)" << endl);
    }
    else
    {
      this->CellMask = (int*)malloc(this->NumberOfCells * sizeof(int));
      CHECK_MALLOC(this->CellMask);
      double* dataTmpMask = (double*)malloc(this->MaximumCells * sizeof(double));

      cdi_set_cur(cdiVar, 0, this->VerticalLevelSelected);
      cdi_get(cdiVar, dataTmpMask, 1);
      // readjust the data
      for (int j = 0; j < this->NumberOfCells; j++)
        this->CellMask[j] = (int)(dataTmpMask[j]);

      free(dataTmpMask);
      vtkDebugMacro(<< "Got data for land/sea mask (2D)" << endl);
    }
    this->gotMask = true;
  }
  return 1;
}

//----------------------------------------------------------------------------
//  Load domain (performance data) variables and integrate them as
//  cell vars. //  Check http://www.kitware.com/blog/home/post/817 if it
//  is possible to adapt to this technique.
//----------------------------------------------------------------------------
bool vtkCDIReader::BuildDomainCellVars()
{
  this->DomainCellVar =
    (double*)malloc(this->NumberOfCells * this->NumberOfDomainVars * sizeof(double));
  vtkUnstructuredGrid* output = GetOutput();
  double* domainTMP = (double*)malloc(this->NumberOfCells * sizeof(double));
  CHECK_MALLOC(this->DomainCellVar);
  CHECK_MALLOC(domainTMP);
  double val = 0;

  int mask_pos = 0;
  int numVars = vlistNvars(this->vlistID);
  for (int i = 0; i < numVars; i++)
    if (!strcmp(this->Internals->cellVars[i].name, this->domain_var_name.c_str()))
      mask_pos = i;

  cdiVar_t* cdiVar = &(this->Internals->cellVars[mask_pos]);
  cdi_set_cur(cdiVar, 0, 0);
  cdi_get(cdiVar, domainTMP, 1);

  for (int j = 0; j < (this->NumberOfDomainVars); j++)
  {
    vtkDoubleArray* domainVar = vtkDoubleArray::New();
    for (int k = 0; k < this->NumberOfCells; k++)
    {
      val = this->DomainVarDataArray[j]->GetComponent(domainTMP[k], 0l);
      this->DomainCellVar[k + (j * this->NumberOfCells)] = val;
    }
    domainVar->SetArray(this->DomainCellVar + (j * this->NumberOfCells), this->CurrentExtraCell, 0,
      vtkDoubleArray::VTK_DATA_ARRAY_FREE);
    domainVar->SetName(this->Internals->domainVars[j].c_str());
    output->GetCellData()->AddArray(domainVar);
  }

  free(domainTMP);
  vtkDebugMacro(<< "Built cell vars from domain data" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Elimate YWrap for Cassini Projection
//----------------------------------------------------------------------------
int vtkCDIReader::EliminateYWrap()
{
  for (int j = 0; j < this->NumberOfCells; j++)
  {
    int* conns = this->OrigConnections + (j * this->PointsPerCell);
    int* modConns = this->ModConnections + (j * this->PointsPerCell);

    int lastk = this->PointsPerCell - 1;
    bool yWrap = false;
    for (int k = 0; k < this->PointsPerCell; k++)
    {
      if (abs(this->PointY[conns[k]] - this->PointY[conns[lastk]]) > 149.5)
        yWrap = true;

      lastk = k;
    }

    if (yWrap)
    {
      for (int k = 0; k < this->PointsPerCell; k++)
        modConns[k] = 0;
    }
    else
    {
      for (int k = 0; k < this->PointsPerCell; k++)
        modConns[k] = conns[k];
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
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro(<< "elim xwrap: singlelayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }
  else
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint * (this->MaximumNVertLevels + 1);
    vtkDebugMacro(<< "elim xwrap: multilayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Elimate XWrap for lon/lat Projection
//----------------------------------------------------------------------------
int vtkCDIReader::EliminateXWrap()
{
  for (int j = 0; j < this->NumberOfCells; j++)
  {
    int* conns = this->OrigConnections + (j * this->PointsPerCell);
    int* modConns = this->ModConnections + (j * this->PointsPerCell);

    int lastk = this->PointsPerCell - 1;
    bool xWrap = false;
    for (int k = 0; k < this->PointsPerCell; k++)
    {
      if (abs(this->PointX[conns[k]] - this->PointX[conns[lastk]]) > 1.0)
        xWrap = true;

      lastk = k;
    }

    if (xWrap)
    {
      for (int k = 0; k < this->PointsPerCell; k++)
        modConns[k] = 0;
    }
    else
    {
      for (int k = 0; k < this->PointsPerCell; k++)
        modConns[k] = conns[k];
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
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro(<< "elim xwrap: singlelayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }
  else
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint * (this->MaximumNVertLevels + 1);
    vtkDebugMacro(<< "elim xwrap: multilayer: setting this->MaximumPoints to "
                  << this->MaximumPoints << endl);
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Add points to vtk data structures
//----------------------------------------------------------------------------
void vtkCDIReader::OutputPoints(bool init)
{
  vtkDebugMacro(<< "In OutputPoints..." << endl);
  float LayerThicknessScaleFactor = 5000.0;

  vtkUnstructuredGrid* output = GetOutput();
  vtkSmartPointer<vtkPoints> points;
  float adjustedLayerThickness = (LayerThickness / LayerThicknessScaleFactor);

  if (InvertZAxis)
    adjustedLayerThickness = -1.0 * (LayerThickness / LayerThicknessScaleFactor);

  vtkDebugMacro(<< "OutputPoints: this->MaximumPoints: " << this->MaximumPoints
                << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
                << " LayerThickness: " << LayerThickness << "ProjectLatLon: " << ProjectLatLon
                << " ShowMultilayerView: " << ShowMultilayerView << endl);
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

  for (int j = 0; j < this->CurrentExtraPoint; j++)
  {
    double x, y, z;
    if (ProjectLatLon)
    {
      x = this->PointX[j] * 180.0 / vtkMath::Pi();
      y = this->PointY[j] * 180.0 / vtkMath::Pi();
      z = 0.0;
    }
    else
    {
      x = this->PointX[j];
      y = this->PointY[j];
      z = this->PointZ[j];
    }

    if (!ShowMultilayerView)
      points->InsertNextPoint(x, y, z);
    else
    {
      double rho = 0.0, rholevel = 0.0, theta = 0.0, phi = 0.0;
      int retval = -1;

      if (!ProjectLatLon && !ProjectCassini)
        if ((x != 0.0) || (y != 0.0) || (z != 0.0))
          retval = CartesianToSpherical(x, y, z, &rho, &phi, &theta);

      if (ProjectLatLon || ProjectCassini)
        z = 0.0;
      else
      {
        if (!retval && ((x != 0.0) || (y != 0.0) || (z != 0.0)))
        {
          rholevel = rho;
          retval = SphericalToCartesian(rholevel, phi, theta, &x, &y, &z);
        }
      }
      points->InsertNextPoint(x, y, z);
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        if (ProjectLatLon || ProjectCassini)
          z = -(double)((this->depth_var[levelNum]) * adjustedLayerThickness);
        else
        {
          if (!retval && ((x != 0.0) || (y != 0.0) || (z != 0.0)))
          {
            rholevel = rho - (adjustedLayerThickness * this->depth_var[levelNum]);
            retval = SphericalToCartesian(rholevel, phi, theta, &x, &y, &z);
          }
        }
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  if (this->reconstruct_new)
  {
    if (this->PointX)
    {
      free(this->PointX);
      this->PointX = NULL;
    }
    if (this->PointY)
    {
      free(this->PointY);
      this->PointY = NULL;
    }
    if (this->PointZ)
    {
      free(this->PointZ);
      this->PointZ = NULL;
    }
  }

  vtkDebugMacro(<< "Leaving OutputPoints..." << endl);
}

//----------------------------------------------------------------------------
// Determine if cell is one of VTK_TRIANGLE, VTK_WEDGE, VTK_QUAD or
// VTK_HEXAHEDRON
//----------------------------------------------------------------------------
unsigned char vtkCDIReader::GetCellType()
{
  // write cell types
  unsigned char cellType = VTK_TRIANGLE;
  switch (this->PointsPerCell)
  {
    case 3:
      if (!ShowMultilayerView)
        cellType = VTK_TRIANGLE;
      else
        cellType = VTK_WEDGE;
      break;
    case 4:
      if (!ShowMultilayerView)
        cellType = VTK_QUAD;
      else
        cellType = VTK_HEXAHEDRON;
      break;
    default:
      break;
  }
  return cellType;
}

//----------------------------------------------------------------------------
//  Add cells to vtk data structures
//----------------------------------------------------------------------------
void vtkCDIReader::OutputCells(bool init)
{
  vtkDebugMacro(<< "In OutputCells..." << endl);
  vtkUnstructuredGrid* output = GetOutput();

  if (init)
    output->Allocate(this->MaximumCells, this->MaximumCells);
  else
  {
    vtkCellArray* cells = output->GetCells();
    cells->Initialize();
    output->Allocate(this->MaximumCells, this->MaximumCells);
  }

  int cellType = GetCellType();
  int val;

  int pointsPerPolygon;
  if (this->ShowMultilayerView)
    pointsPerPolygon = 2 * this->PointsPerCell;
  else
    pointsPerPolygon = this->PointsPerCell;

  vtkDebugMacro(<< "OutputCells: init: " << init << " this->MaximumCells: " << this->MaximumCells
                << " cellType: " << cellType
                << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
                << " LayerThickness: " << LayerThickness << " ProjectLatLon: " << ProjectLatLon
                << " ShowMultilayerView: " << this->ShowMultilayerView);

  std::vector<vtkIdType> polygon(pointsPerPolygon);
  for (int j = 0; j < this->CurrentExtraCell; j++)
  {
    int* conns;
    if (this->ProjectLatLon || this->ProjectCassini)
      conns = this->ModConnections + (j * this->PointsPerCell);
    else
      conns = this->OrigConnections + (j * this->PointsPerCell);

    // singlelayer
    if (!this->ShowMultilayerView)
    {
      // If that min is greater than or equal to this output level,
      // include the cell, otherwise set all points to zero.
      if ((this->gotMask) && (this->IncludeTopography) &&
        (this->CellMask[j] == this->masking_value))
      {
        for (int k = 0; k < this->PointsPerCell; k++)
          polygon[k] = 0;
      }
      else
      {
        for (int k = 0; k < this->PointsPerCell; k++)
          polygon[k] = conns[k];
      }
      output->InsertNextCell(cellType, pointsPerPolygon, &polygon[0]);
    }
    else
    { // multilayer
      // for each level, write the cell
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
        int i = j * this->MaximumNVertLevels;
        if ((this->gotMask) && (this->IncludeTopography) &&
          (this->CellMask[i + levelNum] == this->masking_value))
        {
          // setting all points to zero
          for (int k = 0; k < pointsPerPolygon; k++)
            polygon[k] = 0;
        }
        else
        {
          for (int k = 0; k < this->PointsPerCell; k++)
          {
            val = (conns[k] * (this->MaximumNVertLevels + 1)) + levelNum;
            polygon[k] = val;
          }
          for (int k = 0; k < this->PointsPerCell; k++)
          {
            val = (conns[k] * (this->MaximumNVertLevels + 1)) + levelNum + 1;
            polygon[k + this->PointsPerCell] = val;
          }
        }
        output->InsertNextCell(cellType, pointsPerPolygon, &polygon[0]);
      }
    }
  }

  if (this->gotMask)
  {
    vtkIntArray* mask = vtkIntArray::New();
    mask->SetArray(this->CellMask, this->CurrentExtraCell, 0, vtkIntArray::VTK_DATA_ARRAY_FREE);
    mask->SetName("Land/Sea Mask (wet_c)");
    output->GetCellData()->AddArray(mask);
  }

  if (this->reconstruct_new)
  {
    free(this->ModConnections);
    this->ModConnections = NULL;
    free(this->OrigConnections);
    this->OrigConnections = NULL;
  }

  vtkDebugMacro(<< "Leaving OutputCells..." << endl);
}

//----------------------------------------------------------------------------
//  Load the data for a point variable specified.
//----------------------------------------------------------------------------
int vtkCDIReader::LoadPointVarData(int variableIndex, double dTimeStep)
{
  vtkDebugMacro(<< "In vtkICONReader::LoadPointVarData" << endl);
  cdiVar_t* cdiVar = &(this->Internals->pointVars[variableIndex]);
  int varType = cdiVar->type;

  // Allocate data array for this variable
  if (this->PointVarDataArray[variableIndex] == NULL)
  {
    vtkDebugMacro(<< "allocating data array in vtkICONReader::LoadPointVarData" << endl);
    this->PointVarDataArray[variableIndex] = vtkDoubleArray::New();
    this->PointVarDataArray[variableIndex]->SetName(this->Internals->pointVars[variableIndex].name);
    this->PointVarDataArray[variableIndex]->SetNumberOfTuples(this->MaximumPoints);
    this->PointVarDataArray[variableIndex]->SetNumberOfComponents(1);
  }

  vtkDebugMacro(<< "getting pointer in vtkICONReader::LoadPointVarData" << endl);
  double* dataBlock = this->PointVarDataArray[variableIndex]->GetPointer(0);
  double* dataTmp = (double*)malloc(this->MaximumPoints * sizeof(double));
  vtkDebugMacro(<< "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep), (int)(this->NumberOfTimeSteps - 1));
  vtkDebugMacro(<< "Time: " << timestep << endl);

  // 3D arrays ...
  vtkDebugMacro(<< "Dimensions: " << varType << endl);
  if (varType == 3)
  {
    // singlelayer
    if (!ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, timestep, this->VerticalLevelSelected);
      cdi_get(cdiVar, dataBlock, 1);
      dataBlock[0] = dataBlock[1];
    }
    else
    { // multilayer
      cdi_set_cur(cdiVar, timestep, 0);
      cdi_get(cdiVar, dataTmp, this->MaximumNVertLevels);
      dataTmp[0] = dataTmp[1];
    }
  }
  // 2D arrays ...
  else if (varType == 2)
  {
    if (!ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, timestep, 0);
      cdi_get(cdiVar, dataBlock, 1);
      dataBlock[0] = dataBlock[1];
    }
    else
    {
      cdi_set_cur(cdiVar, timestep, 0);
      cdi_get(cdiVar, dataTmp, this->MaximumNVertLevels);
      dataTmp[0] = dataTmp[1];
    }
  }
  vtkDebugMacro(<< "got point data in vtkICONReader::LoadPointVarData" << endl);

  int i = 0, k = 0;
  if (ShowMultilayerView)
  {
    // put in some dummy points
    for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      dataBlock[levelNum] = dataTmp[this->MaximumNVertLevels + levelNum];

    // write highest level dummy point (duplicate of last level)
    dataBlock[this->MaximumNVertLevels] =
      dataTmp[this->MaximumNVertLevels + this->MaximumNVertLevels - 1];
    vtkDebugMacro(<< "Wrote dummy vtkICONReader::LoadPointVarData" << endl);

    // readjust the data
    for (int j = 0; j < this->NumberOfPoints; j++)
    {
      i = j * (this->MaximumNVertLevels + 1);
      k = j * (this->MaximumNVertLevels);
      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        dataBlock[i++] = dataTmp[j + (levelNum * this->NumberOfPoints)];

      // layer below, which is repeated ...
      dataBlock[i++] = dataTmp[j + ((MaximumNVertLevels - 1) * this->NumberOfPoints)];
    }
  }
  vtkDebugMacro(<< "Wrote next pts vtkICONReader::LoadPointVarData" << endl);
  vtkDebugMacro(<< "this->NumberOfPoints: " << this->NumberOfPoints
                << " this->CurrentExtraPoint: " << this->CurrentExtraPoint << endl);

  // put out data for extra points
  for (int j = this->NumberOfPoints; j < this->CurrentExtraPoint; j++)
  {
    // use map to find out what point data we are using
    if (!ShowMultilayerView)
    {
      k = this->PointMap[j - this->NumberOfPoints];
      dataBlock[j] = dataBlock[k];
    }
    else
    {
      k = this->PointMap[j - this->NumberOfPoints] * this->MaximumNVertLevels;
      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        dataBlock[i++] = dataTmp[k++];

      // for last layer of points, repeat last level's values
      dataBlock[i++] = dataTmp[--k];
    }
  }
  vtkDebugMacro(<< "wrote extra point data in vtkICONReader::LoadPointVarData" << endl);
  free(dataTmp);

  return 1;
}

//----------------------------------------------------------------------------
//  Load the data for a cell variable specified.
//----------------------------------------------------------------------------
int vtkCDIReader::LoadCellVarData(int variableIndex, double dTimeStep)
{
  vtkDebugMacro(<< "In vtkCDIReader::LoadCellVarData" << endl);
  cdiVar_t* cdiVar = &(this->Internals->cellVars[variableIndex]);
  this->CellDataSelected = variableIndex;
  int varType = cdiVar->type;

  // Allocate data array for this variable
  if (this->CellVarDataArray[variableIndex] == NULL)
  {
    this->CellVarDataArray[variableIndex] = vtkDoubleArray::New();
    vtkDebugMacro(<< "Allocated cell var index: " << this->Internals->cellVars[variableIndex].name
                  << endl);
    this->CellVarDataArray[variableIndex]->SetName(this->Internals->cellVars[variableIndex].name);
    this->CellVarDataArray[variableIndex]->SetNumberOfTuples(this->MaximumCells);
    this->CellVarDataArray[variableIndex]->SetNumberOfComponents(1);
  }

  vtkDebugMacro(<< "getting pointer in vtkCDIReader::LoadCellVarData" << endl);
  double* dataBlock = this->CellVarDataArray[variableIndex]->GetPointer(0);
  double* dataTmp = (double*)malloc(this->MaximumCells * sizeof(double));
  vtkDebugMacro(<< "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep), (int)(this->NumberOfTimeSteps - 1));
  vtkDebugMacro(<< "Time: " << timestep << endl);

  // 3D arrays ...
  vtkDebugMacro(<< "Dimensions: " << varType << endl);
  if (varType == 3)
  {
    if (!ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, timestep, this->VerticalLevelSelected);
      cdi_get(cdiVar, dataBlock, 1);
    }
    else
    {
      cdi_set_cur(cdiVar, timestep, 0);
      cdi_get(cdiVar, dataTmp, this->MaximumNVertLevels);

      // readjust the data
      for (int j = 0; j < this->NumberOfCells; j++)
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          dataBlock[i + levelNum] = dataTmp[j + (levelNum * this->NumberOfCells)];
        }
    }
    vtkDebugMacro(<< "Got data for cell var: " << this->Internals->cellVars[variableIndex].name
                  << endl);

    // put out data for extra cells
    for (int j = this->NumberOfCells; j < this->CurrentExtraCell; j++)
    {
      // use map to find out what cell data we are using
      if (!ShowMultilayerView)
      {
        int k = this->CellMap[j - this->NumberOfCells];
        dataBlock[j] = dataBlock[k];
      }
      else
      {
        int i = j * this->MaximumNVertLevels;
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
          dataBlock[i + levelNum] = dataTmp[j + (levelNum * this->NumberOfCells)];
      }
    }
  }
  // 2D arrays ...
  else
  {
    if (!ShowMultilayerView)
    {
      cdi_set_cur(cdiVar, timestep, 0);
      if (varType == 2)
        cdi_get(cdiVar, dataBlock, 1);
      else
        cdi_get(cdiVar, dataBlock, 1);
    }
    else
    {
      cdi_set_cur(cdiVar, timestep, 0);
      if (varType == 2)
        cdi_get(cdiVar, dataTmp, 1);
      else
        cdi_get(cdiVar, dataTmp, 1);

      for (int j = 0; j < +this->NumberOfCells; j++)
        for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
          int i = j * this->MaximumNVertLevels;
          dataBlock[i + levelNum] = dataTmp[j];
        }
    }
    vtkDebugMacro(<< "Got data for cell var: " << this->Internals->cellVars[variableIndex].name
                  << endl);

    // put out data for extra cells
    if (!ShowMultilayerView)
    {
      for (int j = this->NumberOfCells; j < this->CurrentExtraCell; j++)
      {
        // use map to find out what cell data we are using
        int k = this->CellMap[j - this->NumberOfCells];
        dataBlock[j] = dataBlock[k];
      }
    }
    else
    {
      for (int j = this->NumberOfCells; j < this->CurrentExtraCell; j++)
      {
        // use map to find out what cell data we are using
        int k = this->CellMap[j - this->NumberOfCells];
        dataBlock[j] = dataTmp[k];
      }
    }
  }
  vtkDebugMacro(<< "Stored data for cell var: " << this->Internals->cellVars[variableIndex].name
                << endl);
  free(dataTmp);

  return 1;
}

//----------------------------------------------------------------------------
//  Load the data for a domain variable specified
//----------------------------------------------------------------------------
int vtkCDIReader::LoadDomainVarData(int variableIndex)
{
  // This is not very well implemented, also due to the organization of
  // the data available. Needs to be improved together with the modellers.
  vtkDebugMacro(<< "In vtkCDIReader::LoadDomainVarData" << endl);
  string variable = this->Internals->domainVars[variableIndex].c_str();
  this->DomainDataSelected = variableIndex;

  // Allocate data array for this variable
  if (this->DomainVarDataArray[variableIndex] == NULL)
  {
    this->DomainVarDataArray[variableIndex] = vtkDoubleArray::New();
    vtkDebugMacro(<< "Allocated domain var index: " << variable.c_str() << endl);
    this->DomainVarDataArray[variableIndex]->SetName(variable.c_str());
    this->DomainVarDataArray[variableIndex]->SetNumberOfTuples(this->NumberOfDomains);
    this->DomainVarDataArray[variableIndex]->SetNumberOfComponents(1); // 6 components
  }

  for (int i = 0; i < NumberOfDomains; i++)
  {
    string filename;

    if (i < 10)
      filename =
        performance_data_file + convertInt(0) + convertInt(0) + convertInt(0) + convertInt(i);
    else if (i < 100)
      filename = performance_data_file + convertInt(0) + convertInt(0) + convertInt(i);
    else if (i < 1000)
      filename = performance_data_file + convertInt(0) + convertInt(i);
    else
      filename = performance_data_file + convertInt(i);

    vector<string> wordVec;
    vector<string>::iterator k;
    ifstream file(filename.c_str());
    string str, word;
    double temp[1];

    for (int j = 0; j < DomainMask[variableIndex]; j++)
      getline(file, str);

    getline(file, str);
    stringstream ss(str);
    while (ss.good())
    {
      ss >> word;
      strip(word);
      wordVec.push_back(word);
    }
    //  0    1      	2     		3      	4       	5      	6 7		  8
    //  th  L 	name   	#calls 	t_min 	t_ave	t_max 	t_total	   t_total2
    // 00   L		physics   251    	0.4222s  0.9178s  10.52s    03m50s     230.21174
    // for (int l=0; l<6 ; l++)
    //  temp[l] = atof(wordVec.at(2+l).c_str());

    if (strcmp(wordVec.at(1).c_str(), "L"))
      temp[0] = atof(wordVec.at(7).c_str());
    else
      temp[0] = atof(wordVec.at(8).c_str());

    this->DomainVarDataArray[variableIndex]->InsertTuple(i, temp); // for now, we just use t_average
  }

  vtkDebugMacro(<< "Out vtkCDIReader::LoadDomainVarData" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// If the user changes the projection, or singlelayer to
// multilayer, we need to regenerate the geometry.
//----------------------------------------------------------------------------
int vtkCDIReader::RegenerateGeometry()
{
  vtkUnstructuredGrid* output = GetOutput();
  vtkDebugMacro(<< "RegenerateGeometry ..." << endl);
  DestroyData();

  if (!ReadAndOutputGrid(true))
    return 0;

  for (int var = 0; var < this->NumberOfPointVars; var++)
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Point Variable: " << var << endl);
      if (!LoadPointVarData(var, this->DTime))
        return 0;

      output->GetPointData()->AddArray(this->PointVarDataArray[var]);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Cell Variable: " << this->Internals->cellVars[var].name << endl);
      if (!LoadCellVarData(var, this->DTime))
        return 0;

      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
  this->Modified();

  return 1;
}

//----------------------------------------------------------------------------
//  Specify the netCDF dimension names
//-----------------------------------------------------------------------------
int vtkCDIReader::FillVariableDimensions()
{
  int nzaxis = vlistNzaxis(vlistID);
  this->AllDimensions->SetNumberOfValues(0);
  this->VariableDimensions->SetNumberOfValues(nzaxis);
  char nameGridX[20];
  char nameGridY[20];
  char nameLev[20];

  for (int i = 0; i < nzaxis; ++i)
  {
    vtkStdString dimEncoding("(");
    int gridID_l = vlistGrid(vlistID, 0);
    gridInqXname(gridID_l, nameGridX);
    gridInqYname(gridID_l, nameGridY);
    dimEncoding += nameGridX;
    dimEncoding += ", ";
    dimEncoding += nameGridY;
    dimEncoding += ", ";

    int zaxisID_l = vlistZaxis(vlistID, i);
    zaxisInqName(zaxisID_l, nameLev);
    dimEncoding += nameLev;
    dimEncoding += ")";

    this->AllDimensions->InsertNextValue(dimEncoding);
    this->VariableDimensions->SetValue(i, dimEncoding.c_str());
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Set dimensions and rebuild geometry
//-----------------------------------------------------------------------------
void vtkCDIReader::SetDimensions(const char* dimensions)
{
  for (vtkIdType i = 0; i < this->VariableDimensions->GetNumberOfValues(); i++)
    if (this->VariableDimensions->GetValue(i) == dimensions)
      this->dimensionSelection = i;

  if (this->PointDataArraySelection)
    this->PointDataArraySelection->RemoveAllArrays();
  if (this->CellDataArraySelection)
    this->CellDataArraySelection->RemoveAllArrays();
  if (this->DomainDataArraySelection)
    this->DomainDataArraySelection->RemoveAllArrays();

  this->reconstruct_new = true;
  this->DestroyData();
  this->RegenerateVariables();
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
//  Callback if the user selects a variable.
//----------------------------------------------------------------------------
void vtkCDIReader::SelectionCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkCDIReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
//  Return the output.
//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkCDIReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
//  Returns the output given an id.
//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkCDIReader::GetOutput(int idx)
{
  if (idx)
    return NULL;
  else
    return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
//  Get number of point arrays.
//----------------------------------------------------------------------------
int vtkCDIReader::GetNumberOfPointArrays()
{

  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
// Get number of cell arrays.
//----------------------------------------------------------------------------
int vtkCDIReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}
//----------------------------------------------------------------------------
// Get number of domain arrays.
//----------------------------------------------------------------------------
int vtkCDIReader::GetNumberOfDomainArrays()
{
  return this->DomainDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
// Make all point selections available.
//----------------------------------------------------------------------------
void vtkCDIReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
// Make all point selections unavailable.
//----------------------------------------------------------------------------
void vtkCDIReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
// Make all cell selections available.
//----------------------------------------------------------------------------
void vtkCDIReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
// Make all cell selections unavailable.
//----------------------------------------------------------------------------
void vtkCDIReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
// Make all domain selections available.
//----------------------------------------------------------------------------
void vtkCDIReader::EnableAllDomainArrays()
{
  this->DomainDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
// Make all domain selections unavailable.
//----------------------------------------------------------------------------
void vtkCDIReader::DisableAllDomainArrays()
{
  this->DomainDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
// Get name of indexed point variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetPointArrayName(int index)
{
  return (const char*)(this->Internals->pointVars[index].name);
}

//----------------------------------------------------------------------------
// Get status of named point variable selection
//----------------------------------------------------------------------------
int vtkCDIReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
// Set status of named point variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    this->PointDataArraySelection->EnableArray(name);
  else
    this->PointDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
// Get name of indexed cell variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetCellArrayName(int index)
{
  return (const char*)(this->Internals->cellVars[index].name);
}

//----------------------------------------------------------------------------
// Get status of named cell variable selection.
//----------------------------------------------------------------------------
int vtkCDIReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
// Set status of named cell variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    this->CellDataArraySelection->EnableArray(name);
  else
    this->CellDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
// Get name of indexed domain variable
//----------------------------------------------------------------------------
const char* vtkCDIReader::GetDomainArrayName(int index)
{
  return (const char*)(this->Internals->domainVars[index].c_str());
}

//----------------------------------------------------------------------------
// Get status of named domain variable selection.
//----------------------------------------------------------------------------
int vtkCDIReader::GetDomainArrayStatus(const char* name)
{
  return this->DomainDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
// Set status of named domain variable selection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetDomainArrayStatus(const char* name, int status)
{
  if (status)
    this->DomainDataArraySelection->EnableArray(name);
  else
    this->DomainDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
//  Set vertical level to be viewed.
//----------------------------------------------------------------------------
void vtkCDIReader::SetVerticalLevel(int level)
{
  this->VerticalLevelSelected = level;
  vtkDebugMacro(<< "Set VerticalLevelSelected to: " << level);
  vtkDebugMacro(<< "InfoRequested?: " << this->InfoRequested);

  if (!this->InfoRequested)
    return;

  if (!this->DataRequested)
    return;

  for (int var = 0; var < this->NumberOfPointVars; var++)
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Point Variable: " << this->Internals->pointVars[var].name << endl);
      LoadPointVarData(var, this->DTime);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Cell Variable: " << this->Internals->cellVars[var].name << endl);
      LoadCellVarData(var, this->DTime);
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}

//----------------------------------------------------------------------------
//  Set layer thickness for multilayer view.
//----------------------------------------------------------------------------
void vtkCDIReader::SetLayerThickness(int val)
{
  if (LayerThickness != val)
  {
    LayerThickness = val;
    vtkDebugMacro(<< "SetLayerThickness: LayerThickness set to " << LayerThickness << endl);

    if (ShowMultilayerView)
    {
      if (!this->InfoRequested)
        return;

      if (!this->DataRequested)
        return;

      RegenerateGeometry();
    }
  }
}

//----------------------------------------------------------------------------
//  Enable/disable missing values and check the variables displayed.
//----------------------------------------------------------------------------
void vtkCDIReader::EnableMissingValue(bool val)
{
  this->RemoveMissingValues = val;

  if (!this->InfoRequested)
    return;

  if (!this->DataRequested)
    return;

  for (int var = 0; var < this->NumberOfPointVars; var++)
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Point Variable: " << this->Internals->pointVars[var].name << endl);
      LoadPointVarData(var, this->DTime);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Cell Variable: " << this->Internals->cellVars[var].name << endl);
      LoadCellVarData(var, this->DTime);
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}

//----------------------------------------------------------------------------
//  Set missing values and check the variables displayed.
//----------------------------------------------------------------------------
void vtkCDIReader::SetMissingValue(double val)
{
  this->MissingValue = val;

  if (!this->InfoRequested)
    return;

  if (!this->DataRequested)
    return;

  for (int var = 0; var < this->NumberOfPointVars; var++)
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Point Variable: " << this->Internals->pointVars[var].name << endl);
      LoadPointVarData(var, this->DTime);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDebugMacro(<< "Loading Cell Variable: " << this->Internals->cellVars[var].name << endl);
      LoadCellVarData(var, this->DTime);
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}

//----------------------------------------------------------------------------
// Set to lat/lon (equidistant cylindrical) projection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetProjectLatLon(bool val)
{
  if (val)
    ProjectCassini = false;

  if (ProjectLatLon != val)
  {
    ProjectLatLon = val;
    this->reconstruct_new = true;

    if (!this->InfoRequested)
      return;

    if (!this->DataRequested)
      return;

    RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set to Cassini projection.
//----------------------------------------------------------------------------
void vtkCDIReader::SetProjectCassini(bool val)
{
  if (ProjectCassini != val)
  {
    ProjectCassini = val;
    this->reconstruct_new = true;

    if (!this->InfoRequested)
      return;

    if (!this->DataRequested)
      return;

    RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Invert the visualization.
//----------------------------------------------------------------------------
void vtkCDIReader::SetInvertZAxis(bool val)
{
  if (InvertZAxis != val)
  {
    InvertZAxis = val;

    if (!this->InfoRequested)
      return;

    if (!this->DataRequested)
      return;

    RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set visualization with topography/bathymetrie.
//----------------------------------------------------------------------------
void vtkCDIReader::SetTopography(bool val)
{
  if (IncludeTopography != val)
  {
    IncludeTopography = val;

    if (!this->InfoRequested)
      return;

    if (!this->DataRequested)
      return;

    RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
// Set visualization with inverted topography/bathymetrie.
//----------------------------------------------------------------------------
void vtkCDIReader::InvertTopography(bool val)
{
  if (val)
    this->masking_value = 1.0;
  else
    this->masking_value = 0.0;

  if (!this->InfoRequested)
    return;

  if (!this->DataRequested)
    return;

  RegenerateGeometry();
}

//----------------------------------------------------------------------------
//  Set view to be multilayered view.
//----------------------------------------------------------------------------
void vtkCDIReader::SetShowMultilayerView(bool val)
{
  if (ShowMultilayerView != val)
  {
    ShowMultilayerView = val;

    if (!this->InfoRequested)
      return;

    if (!this->DataRequested)
      return;

    RegenerateGeometry();
  }
}

//----------------------------------------------------------------------------
//  Print self.
//----------------------------------------------------------------------------
void vtkCDIReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NULL") << "\n";
  os << indent << "VariableDimensions: " << this->VariableDimensions << endl;
  os << indent << "AllDimensions: " << this->AllDimensions << endl;
  os << indent << "this->NumberOfPointVars: " << this->NumberOfPointVars << "\n";
  os << indent << "this->NumberOfCellVars: " << this->NumberOfCellVars << "\n";
  os << indent << "this->NumberOfDomainVars: " << this->NumberOfDomainVars << "\n";
  os << indent << "this->MaximumPoints: " << this->MaximumPoints << "\n";
  os << indent << "this->MaximumCells: " << this->MaximumCells << "\n";
  os << indent << "ProjectLatLon: " << (this->ProjectLatLon ? "ON" : "OFF") << endl;
  os << indent << "ProjectCassini: " << (this->ProjectCassini ? "ON" : "OFF") << endl;
  os << indent << "VerticalLevelRange: " << this->VerticalLevelRange << "\n";
  os << indent << "ShowMultilayerView: " << (this->ShowMultilayerView ? "ON" : "OFF") << endl;
  os << indent << "InvertZ: " << (this->InvertZAxis ? "ON" : "OFF") << endl;
  os << indent << "UseTopography: " << (this->IncludeTopography ? "ON" : "OFF") << endl;
  os << indent << "SetInvertTopography: " << (this->invertedTopography ? "ON" : "OFF") << endl;
  os << indent << "LayerThicknessRange: " << this->LayerThicknessRange[0] << ","
     << this->LayerThicknessRange[1] << endl;
}
