// -*- c++ -*-
/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    vtkCDIReader.h
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

#ifndef vtkCDIReader_h
#define vtkCDIReader_h

#define DEBUG 0
#define MAX_VARS 100

#include "vtkCDIReaderModule.h"
#include "vtkDataArraySelection.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCallbackCommand;
class vtkDoubleArray;
class vtkStringArray;

class VTKCDIREADER_EXPORT vtkCDIReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCDIReader* New();
  vtkTypeMacro(vtkCDIReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetFileName(const char* val);

  vtkGetMacro(MaximumCells, int);
  vtkGetMacro(MaximumPoints, int);
  vtkGetMacro(NumberOfCellVars, int);
  vtkGetMacro(NumberOfPointVars, int);

  vtkStringArray* VariableDimensions;
  vtkStringArray* AllDimensions;
  vtkSmartPointer<vtkIntArray> LoadingDimensions;
  void SetDimensions(const char* dimensions);
  vtkStringArray* GetAllVariableArrayNames();
  vtkSmartPointer<vtkStringArray> AllVariableArrayNames;
  vtkGetObjectMacro(AllDimensions, vtkStringArray);
  vtkGetObjectMacro(VariableDimensions, vtkStringArray);

  int GetNumberOfVariableArrays() { return this->GetNumberOfCellArrays(); }
  const char* GetVariableArrayName(int idx) { return this->GetCellArrayName(idx); }
  int GetVariableArrayStatus(const char* name) { return this->GetCellArrayStatus(name); }
  void SetVariableArrayStatus(const char* name, int status)
  {
    this->SetCellArrayStatus(name, status);
  }

  int GetNumberOfPointArrays() { return this->PointDataArraySelection->GetNumberOfArrays(); }
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name)
  {
    return this->PointDataArraySelection->ArrayIsEnabled(name);
  }
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays() { this->PointDataArraySelection->DisableAllArrays(); }
  void EnableAllPointArrays() { this->PointDataArraySelection->EnableAllArrays(); }

  int GetNumberOfCellArrays() { return this->CellDataArraySelection->GetNumberOfArrays(); }
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name)
  {
    return this->CellDataArraySelection->ArrayIsEnabled(name);
  }
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays() { this->CellDataArraySelection->DisableAllArrays(); }
  void EnableAllCellArrays() { this->CellDataArraySelection->EnableAllArrays(); }

  int GetNumberOfDomainArrays() { return this->DomainDataArraySelection->GetNumberOfArrays(); }
  const char* GetDomainArrayName(int index);
  int GetDomainArrayStatus(const char* name)
  {
    return this->DomainDataArraySelection->ArrayIsEnabled(name);
  }
  void SetDomainArrayStatus(const char* name, int status);
  void DisableAllDomainArrays() { this->DomainDataArraySelection->DisableAllArrays(); }
  void EnableAllDomainArrays() { this->DomainDataArraySelection->EnableAllArrays(); }

  int GetNumberOfDomains() { return this->NumberOfDomains; }
  int GetNumberOfDomainsVars() { return this->NumberOfDomainVars; }
  bool SupportDomainData() { return (this->HaveDomainData && this->HaveDomainVariable); }

  void SetVerticalLevel(int level);
  vtkGetVector2Macro(VerticalLevelRange, int);
  vtkGetMacro(VerticalLevelSelected, int);

  void SetLayerThickness(int val);
  vtkGetVector2Macro(LayerThicknessRange, int);
  vtkGetMacro(LayerThickness, int);

  void SetProjection(int val);
  vtkGetMacro(ProjectionMode, int);

  void SetDoublePrecision(bool val);
  vtkGetMacro(DoublePrecision, bool);

  void SetInvertZAxis(bool val);
  vtkGetMacro(InvertZAxis, bool);

  void SetTopography(bool val);
  vtkGetMacro(IncludeTopography, bool);

  void InvertTopography(bool val);
  vtkGetMacro(InvertedTopography, bool);

  void SetShowMultilayerView(bool val);
  vtkGetMacro(ShowMultilayerView, bool);

#ifdef PARAVIEW_USE_MPI
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);
#endif

protected:
  vtkCDIReader();
  ~vtkCDIReader() override;
  int OpenFile();
  void DestroyData();
  void SetDefaults();
  int CheckForMaskData();
  int GetVars();
  int ReadAndOutputGrid(bool init);
  int ReadAndOutputVariableData();
  int ReadTimeUnits(const char* Name);
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
  int LoadClonClatVars();
  int MirrorMesh();
  bool BuildDomainCellVars();
  void RemoveDuplicates(
    double* PointLon, double* PointLat, int temp_nbr_vertices, int* triangle_list, int* nbr_cells);
  long GetPartitioning(int piece, int numPieces, int numCellsPerLevel, int numPointsPerCell,
    int& beginPoint, int& endPoint, int& beginCell, int& endCell);
  void SetupPointConnectivity();

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  static void SelectionCallback(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid),
    void* clientdata, void* vtkNotUsed(calldata))
  {
    static_cast<vtkCDIReader*>(clientdata)->Modified();
  }

  int GetDims();
  int ReadHorizontalGridData();
  int ReadVerticalGridData();
  int FillVariableDimensions();
  int RegenerateVariables();

#ifdef PARAVIEW_USE_MPI
  vtkMultiProcessController* Controller;
#endif

  int NumberOfProcesses;
  bool InvertedTopography;
  bool FilenameSet;
  float MaskingValue;
  int BeginPoint, EndPoint, BeginCell, EndCell;
  int Piece, NumPieces;
  int NumberLocalCells;
  int NumberAllCells;
  int NumberLocalPoints;
  int NumberAllPoints;
  bool Decomposition;

  std::string FileName;
  std::string FileNameGrid;
  std::string FileSeriesFirstName;
  int* VariableType;
  int NumberOfTimeSteps;
  double DTime;
  int FileSeriesNumber;
  int NumberOfFiles;
  double TStepDistance;

  vtkCallbackCommand* SelectionObserver;
  bool InfoRequested;
  bool DataRequested;
  bool Grib;

  vtkDataArraySelection* CellDataArraySelection;
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* DomainDataArraySelection;

  vtkDataArray** CellVarDataArray;
  vtkDataArray** PointVarDataArray;
  vtkDoubleArray** DomainVarDataArray;

  int VerticalLevelSelected;
  int VerticalLevelRange[2];
  int CellDataSelected;
  int PointDataSelected;
  int DomainDataSelected;
  int LayerThickness;
  int LayerThicknessRange[2];

  int DimensionSelection;
  bool InvertZAxis;
  bool GotMask, AddCoordinateVars;
  int ProjectionMode;
  bool DoublePrecision;
  bool ShowMultilayerView;
  bool IncludeTopography;
  bool HaveDomainData;
  bool HaveDomainVariable;
  bool BuildDomainArrays;
  std::string DomainVarName;
  std::string DomainDimension;
  std::string PerformanceDataFile;

  int MaximumNVertLevels;
  int NumberOfCells;
  int NumberOfVertices;
  int NumberOfPoints;
  int NumberOfTriangles;
  int NumberOfDomains;
  int PointsPerCell;
  bool ReconstructNew;
  bool NeedHorizontalGridFile;
  bool NeedVerticalGridFile;

  double* CLonVertices;
  double* CLatVertices;
  double* CLon;
  double* CLat;
  double* DepthVar;
  double* PointX;
  double* PointY;
  double* PointZ;
  int ModNumPoints;
  int ModNumCells;
  int* OrigConnections;
  int* ModConnections;
  int* CellMask;
  int* DomainMask;
  double* DomainCellVar;
  int MaximumCells;
  int MaximumPoints;
  int* VertexIds;

  int NumberOfCellVars;
  int NumberOfPointVars;
  int NumberOfDomainVars;
  double* PointVarData;
  bool GridReconstructed;

  int StreamID;
  int VListID;
  int GridID;
  int ZAxisID;
  int SurfID;

  char* TimeUnits;
  char* Calendar;
  vtkSmartPointer<vtkUnstructuredGrid> Output;

private:
  vtkCDIReader(const vtkCDIReader&) = delete;
  void operator=(const vtkCDIReader&) = delete;

  class Internal;
  Internal* Internals;

  template <typename ValueType>
  int LoadCellVarDataTemplate(int variable, double dTime, vtkDataArray* dataArray);
  template <typename ValueType>
  int LoadPointVarDataTemplate(int variable, double dTime, vtkDataArray* dataArray);
};

#endif
