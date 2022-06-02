/*=========================================================================

   Program: ParaView
   Module:  vtkCDIReader.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

/*-------------------------------------------------------------------------
   Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
  -------------------------------------------------------------------------*/

#ifndef vtkCDIReader_h
#define vtkCDIReader_h

#include "vtkCDIReaderModule.h" // for export macro
#include "vtkUnstructuredGridAlgorithm.h"

#include "vtkDataArraySelection.h" // for ivars
#include "vtkSmartPointer.h"       // for ivars
#include "vtkStringArray.h"        // for ivars

#include "projections.h" // for projection enum

#include <memory> // for unique_ptr
#include <vector> // for std::vector

class vtkCallbackCommand;
class vtkDoubleArray;
class vtkFieldData;
class vtkMultiProcessController;

/**
 *
 * @class vtkCDIReader
 * @brief reads ICON/CDI netCDF data sets
 *
 * vtkCDIReader is based on the vtk MPAS netCDF reader developed by
 * Christine Ahrens (cahrens@lanl.gov). The plugin reads all ICON/CDI
 * netCDF data sets with point and cell variables, both 2D and 3D. It allows
 * spherical (standard), as well as equidistant cylindrical and Cassini projection.
 * 3D data can be visualized using slices, as well as 3D unstructured mesh. If
 * bathymetry information (wet_c) is present in the data, this can be used for
 * masking out continents. For more information, also check out our ParaView tutorial:
 * https://www.dkrz.de/Nutzerportal-en/doku/vis/sw/paraview
 *
 * @section caveats Caveats
 * The integrated visualization of performance data is not yet fully developed
 * and documented. If interested in using it, see the following presentation
 * https://www.dkrz.de/about/media/galerie/Vis/performance/perf-vis
 * and/or contact Niklas Roeber at roeber@dkrz.de
 *
 * @section thanks Thanks
 * Thanks to Uwe Schulzweida for the CDI code (uwe.schulzweida@mpimet.mpg.de)
 * Thanks to Moritz Hanke for the sorting code (hanke@dkrz.de)
 */

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

  vtkSmartPointer<vtkStringArray> VariableDimensions;
  vtkSmartPointer<vtkStringArray> AllDimensions;
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

  void SetMaskingVariable(const char* name);

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

  void SetLayer0Offset(double val);
  vtkGetVector2Macro(Layer0OffsetRange, double);
  vtkGetMacro(Layer0Offset, double);

  void SetProjection(int val);
  vtkGetMacro(ProjectionMode, int);

  void SetDoublePrecision(bool val);
  vtkGetMacro(DoublePrecision, bool);

  void SetWrapping(bool val);
  vtkGetMacro(WrapOn, bool);

  void SetShowClonClat(bool val);
  vtkGetMacro(ShowClonClat, bool);

  void SetInvertZAxis(bool val);
  vtkGetMacro(InvertZAxis, bool);

  void SetUseMask(bool val);
  vtkGetMacro(UseMask, bool);

  void SetInvertMask(bool val);
  vtkGetMacro(InvertMask, bool);

  void SetSkipGrid(bool val);
  vtkGetMacro(SkipGrid, bool);

  void SetUseCustomMaskValue(bool val);
  vtkGetMacro(UseCustomMaskValue, bool);

  void SetCustomMaskValue(double val);
  vtkGetMacro(CustomMaskValue, double);

  void SetShowMultilayerView(bool val);
  vtkGetMacro(ShowMultilayerView, bool);

  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

protected:
  vtkCDIReader();
  ~vtkCDIReader() override;

  int OpenFile();
  void DestroyData();
  void SetDefaults();
  int CheckForMaskData();
  int AddMaskHalo();
  int GetVars();
  int ReadAndOutputGrid(bool init);
  int ReadAndOutputVariableData();
  int ReadTimeUnits(const char* Name);
  vtkSmartPointer<vtkDoubleArray> ReadTimeAxis();
  int BuildVarArrays();
  int AllocSphereGeometry();
  int AllocLatLonGeometry();
  int Wrap(int axis);
  void OutputPoints(bool init);
  void OutputCells(bool init);
  void InsertPolyhedron(std::vector<vtkIdType> polygon);
  unsigned char GetCellType();
  void LoadGeometryData(int var, double dTime);
  int LoadPointVarData(int variable, double dTime);
  int LoadCellVarData(int variable, double dTime);
  int LoadDomainVarData(int variable);
  int ReplaceFillWithNan(const int varID, vtkDataArray* dataArray);
  int RegenerateGeometry();
  int ConstructGridGeometry();
  void GuessGridFile();
  int LoadClonClatVars();
  int AddClonClatHalo();
  int MirrorMesh();
  bool BuildDomainCellVars();
  void RemoveDuplicates(
    double* PointLon, double* PointLat, int temp_nbr_vertices, int* triangle_list, int* nbr_cells);
  long GetPartitioning(int piece, int numPieces, int numCellsPerLevel, int numPointsPerCell,
    int& beginPoint, int& endPoint, int& beginCell, int& endCell);
  void SetupPointConnectivity();

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ResetVarDataArrays();

  void OutputTimeAxis(vtkInformation* outInfo, const vtkSmartPointer<vtkDoubleArray>& timeValues);

  void GetFileSeriesInformation(vtkInformation* outInfo);

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  static void SelectionCallback(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid),
    void* clientdata, void* vtkNotUsed(calldata))
  {
    static_cast<vtkCDIReader*>(clientdata)->Modified();
  }

  int GetTimeIndex(double DataTimeStep);
  int GetDims();
  int ReadHorizontalGridData();
  int ReadVerticalGridData();
  int FillGridDimensions();
  int RegenerateVariables();
  int AddMirrorPointX(int index, double dividerX, double offset);
  int AddMirrorPointY(int index, double dividerY, double offset);

  vtkMultiProcessController* Controller;

  bool Initialized;

  int NumberOfProcesses;
  double CustomMaskValue;
  int BeginPoint, EndPoint, BeginCell, EndCell;
  int Piece, NumPieces;
  int NumberLocalCells;
  int NumberAllCells;
  int NumberLocalPoints;
  int NumberAllPoints;
  bool Decomposition;
  long FirstDay;
  int ModNumPoints;
  int ModNumCells;
  unsigned int CurrentExtraPoint;     // current extra point
  unsigned int CurrentExtraCell;      // current extra cell
  std::vector<unsigned int> CellMap;  // maps from added cell to original cell #
  std::vector<unsigned int> PointMap; // maps from added point to original point #

  std::string FileName;
  std::string FileNameGrid;
  std::string FileNameGridSelect;
  std::string FileSeriesFirstName;
  std::string MaskingVarname;
  int NumberOfTimeSteps;
  int NumberOfAllTimeSteps;
  std::vector<int> TimeSeriesTimeSteps;
  bool TimeSeriesTimeStepsAllSet;
  bool TimeSet;
  double DTime;
  std::vector<double> TimeSteps;
  int FileSeriesNumber;
  int NumberOfFiles;
  double Bloat;

  bool UseMask;
  bool InvertMask;
  bool GotMask;
  bool UseCustomMaskValue;

  bool SkipGrid;

  vtkNew<vtkCallbackCommand> SelectionObserver;
  bool InfoRequested;
  bool DataRequested;
  bool Grib;

  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> DomainDataArraySelection;

  vtkNew<vtkFieldData> CellVarDataArray;
  vtkNew<vtkFieldData> PointVarDataArray;
  vtkNew<vtkFieldData> DomainVarDataArray;

  int VerticalLevelSelected;
  int VerticalLevelRange[2];
  int CellDataSelected;
  int PointDataSelected;
  int DomainDataSelected;
  int LayerThickness;
  int LayerThicknessRange[2];
  double Layer0Offset;
  double Layer0OffsetRange[2];

  int DimensionSelection;
  bool InvertZAxis;
  bool AddCoordinateVars;
  projection::Projection ProjectionMode;
  bool DoublePrecision;
  bool ShowClonClat;
  bool ShowMultilayerView;
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
  bool WrapOn;

  std::vector<double> CLon;
  std::vector<double> CLat;
  double* DepthVar;
  std::vector<double> PointX;
  std::vector<double> PointY;
  std::vector<double> PointZ;
  std::vector<int> OrigConnections;
  std::vector<int> ModConnections;
  std::vector<bool> CellMask;
  double* DomainCellVar;
  int MaximumCells;
  int MaximumPoints;
  std::vector<int> VertexIds;

  int NumberOfCellVars;
  int NumberOfPointVars;
  int NumberOfDomainVars;
  bool GridReconstructed;

  int StreamID;
  int VListID;
  int GridID;
  int ZAxisID;
  int SurfID;

  std::string TimeUnits;
  std::string Calendar;
  vtkSmartPointer<vtkDoubleArray> ClonArray;
  vtkSmartPointer<vtkDoubleArray> ClatArray;
  vtkNew<vtkUnstructuredGrid> Output;

private:
  vtkCDIReader(const vtkCDIReader&) = delete;
  void operator=(const vtkCDIReader&) = delete;

  class Internal;
  std::unique_ptr<Internal> Internals;

  template <typename ValueType>
  int LoadCellVarDataTemplate(int variable, double dTime, vtkDataArray* dataArray);
  template <typename ValueType>
  int LoadPointVarDataTemplate(int variable, double dTime, vtkDataArray* dataArray);
};

#endif
