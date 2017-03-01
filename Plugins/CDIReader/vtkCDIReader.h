// -*- c++ -*-
/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    vtkCDIReader.h
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

#ifndef __vtkCDIReader_h
#define __vtkCDIReader_h

#define MAX_VARS 100
#define MAX_VAR_NAME 100

#ifdef __linux__
// linux code goes here
#elif _WIN32
#define VTKIONETCDF_EXPORT __declspec(dllexport)
#endif

#include "vtkIONetCDFModule.h"
#include "vtkIntArray.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include <string>
#include <vtkExtractSelection.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSmartPointer.h>

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "cdi.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStdString;
class vtkStringArray;

typedef struct
{
  int streamID;
  int varID;
  int gridID;
  int zaxisID;
  int gridsize;
  int nlevel;
  int type;
  int const_time;

  int timestep;
  int levelID;

  char name[CDI_MAX_NAME];
} cdiVar_t;

struct point
{
  double lon;
  double lat;
};

struct point_with_index
{
  point p;
  int i;
};

class VTKIONETCDF_EXPORT vtkCDIReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCDIReader* New();
  vtkTypeMacro(vtkCDIReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  vtkGetMacro(MaximumCells, int);
  vtkGetMacro(MaximumPoints, int);
  vtkGetMacro(NumberOfCellVars, int);
  vtkGetMacro(NumberOfPointVars, int);

  vtkUnstructuredGrid* GetOutput();
  vtkUnstructuredGrid* GetOutput(int index);

  vtkStringArray* VariableDimensions;
  vtkStringArray* AllDimensions;
  vtkSmartPointer<vtkIntArray> LoadingDimensions;
  void SetDimensions(const char* dimensions);
  vtkStringArray* GetAllVariableArrayNames();
  vtkSmartPointer<vtkStringArray> AllVariableArrayNames;
  vtkGetObjectMacro(AllDimensions, vtkStringArray);
  vtkGetObjectMacro(VariableDimensions, vtkStringArray);

  int GetNumberOfVariableArrays() { return GetNumberOfCellArrays(); };
  const char* GetVariableArrayName(int idx) { return GetCellArrayName(idx); };
  int GetVariableArrayStatus(const char* name) { return GetCellArrayStatus(name); };
  void SetVariableArrayStatus(const char* name, int status) { SetCellArrayStatus(name, status); };

  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();

  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();

  int GetNumberOfDomainArrays();
  const char* GetDomainArrayName(int index);
  int GetDomainArrayStatus(const char* name);
  void SetDomainArrayStatus(const char* name, int status);
  void DisableAllDomainArrays();
  void EnableAllDomainArrays();

  int getNumberOfDomains() { return NumberOfDomains; };
  int getNumberOfDomainsVars() { return NumberOfDomainVars; };
  bool SupportDomainData() { return (haveDomainData && haveDomainVariable); };

  void SetVerticalLevel(int level);
  vtkGetVector2Macro(VerticalLevelRange, int);

  void SetLayerThickness(int val);
  vtkGetVector2Macro(LayerThicknessRange, int);

  void SetProjectLatLon(bool val);
  vtkGetMacro(ProjectLatLon, bool);

  void SetProjectCassini(bool val);
  vtkGetMacro(ProjectCassini, bool);

  void SetMissingValue(double val);
  void EnableMissingValue(bool val);
  vtkGetMacro(MissingValue, double);

  void SetInvertZAxis(bool val);
  vtkGetMacro(InvertZAxis, bool);

  void SetTopography(bool val);
  vtkGetMacro(IncludeTopography, bool);

  void InvertTopography(bool val);
  vtkGetMacro(invertedTopography, bool);

  void SetShowMultilayerView(bool val);
  vtkGetMacro(ShowMultilayerView, bool);

protected:
  vtkCDIReader();
  ~vtkCDIReader();
  void DestroyData();
  void SetDefaults();
  bool invertedTopography;
  int CheckForMaskData();
  float masking_value;
  int GetDims();
  int GetVars();
  int ReadAndOutputGrid(bool init);
  int ReadAndOutputVariableData();
  int BuildVarArrays();
  int AllocSphereGeometry();
  int AllocLatLonGeometry();
  int EliminateXWrap();
  int EliminateYWrap();
  void OutputPoints(bool init);
  void OutputCells(bool init);
  unsigned char GetCellType();
  void LoadGeometryData(int var, double dTime);
  int LoadPointVarData(int variable, double dTime);
  int LoadCellVarData(int variable, double dTime);
  int LoadDomainVarData(int variable);
  int RegenerateGeometry();
  int ConstructGridGeometry();
  int MirrorMesh();
  bool BuildDomainCellVars();
  void Remove_Duplicates(
    double* PointLon, double* PointLat, int temp_nbr_vertices, int* vertexID, int* nbr_cells);
  void cellMask(const char* arg1);

  char* FileName;
  vtkStdString* VariableName;
  int* VariableType;
  int NumberOfTimeSteps;
  double* TimeSteps;
  double DTime;

  vtkCallbackCommand* SelectionObserver;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  static void SelectionCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  bool InfoRequested;
  bool DataRequested;

  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  vtkDataArraySelection* DomainDataArraySelection;

  vtkDoubleArray** CellVarDataArray;   // Actual data arrays
  vtkDoubleArray** PointVarDataArray;  // Actual data arrays
  vtkDoubleArray** DomainVarDataArray; // Actual data arrays

  int VerticalLevelSelected;
  int VerticalLevelRange[2];
  int CellDataSelected;
  int PointDataSelected;
  int DomainDataSelected;
  int LayerThickness;
  int LayerThicknessRange[2];

  int FillVariableDimensions();
  int dimensionSelection;
  int RegenerateVariables();
  double MissingValue;
  bool RemoveMissingValues;
  bool InvertZAxis;
  bool gotMask;
  bool ProjectLatLon, ProjectCassini;
  bool ShowMultilayerView;
  bool IncludeTopography;
  bool haveDomainData, haveDomainVariable, buildDomainArrays;
  std::string domain_var_name;
  std::string domain_dimension;
  std::string performance_data_file;

  // geometry
  int MaximumNVertLevels;
  int NumberOfCells;
  int NumberOfVertices;
  int NumberOfPoints;
  int NumberOfTriangles;
  int NumberOfDomains;
  int PointsPerCell;
  int CurrentExtraPoint; // current extra point
  int CurrentExtraCell;  // current extra  cell
  bool reconstruct_new;

  double* clon_vertices;
  double* clat_vertices;
  double* depth_var;
  double* PointX; // x coord of point
  double* PointY; // y coord of point
  double* PointZ; // z coord of point
  int ModNumPoints;
  int ModNumCells;
  int* OrigConnections; // original connections
  int* ModConnections;  // modified connections
  int* CellMap;         // maps from added cell to original cell #
  int* CellMask;
  int* DomainMask;        // in which line is which domain variable
  double* DomainCellVar;  // in which line is which domain variable
  int* PointMap;          // maps from added point to original point #
  int* MaximumLevelPoint; //
  int MaximumCells;       // max cells
  int MaximumPoints;      // max points

  // vars
  int NumberOfCellVars;
  int NumberOfPointVars;
  int NumberOfDomainVars;
  double* PointVarData;
  bool grid_reconstructed;

  // cdi vars
  int streamID;
  int vlistID;
  int gridID;
  int zaxisID;
  int surfID;

private:
  vtkCDIReader(const vtkCDIReader&);
  void operator=(const vtkCDIReader&);
  class Internal;
  Internal* Internals;
};

#endif
