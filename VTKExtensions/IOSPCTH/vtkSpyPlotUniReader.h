/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotUniReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpyPlotUniReader
 * @brief   Read SPCTH Spy Plot file format
 *
 * vtkSpyPlotUniReader is a reader that reads SPCTH Spy Plot file binary format
 * that describes part of a dataset (in the case that the simulation consists of
 * more than 1 file) or the entire simulation.
 * The reader supports both Spy dataset types: flat mesh and AMR
 * (Adaptive Mesh Refinement).
 *
 * @par Implementation Details:
 * Class was extracted from vtkSpyPlotReader.cxx and is a helper to that
 * class.  Note the grids in the reader may have bad ghost cells that will
 * need to be taken into consideration in terms of both geometry and
 * cell data
 *-----------------------------------------------------------------------------
 *=============================================================================
*/

#ifndef vtkSpyPlotUniReader_h
#define vtkSpyPlotUniReader_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports
class vtkSpyPlotBlock;
class vtkDataArraySelection;
class vtkDataArray;
class vtkFloatArray;
class vtkIntArray;
class vtkUnsignedCharArray;
class vtkSpyPlotIStream;

class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotUniReader : public vtkObject
{
public:
  vtkTypeMacro(vtkSpyPlotUniReader, vtkObject);
  static vtkSpyPlotUniReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set and get the Binary SpyPlot File name the reader will process
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  virtual void SetCellArraySelection(vtkDataArraySelection* da);
  //@}

  /**
   * Reads the basic information from the file such as the header, number
   * of fields, etc.
   */
  virtual int ReadInformation();

  /**
   * Make sure that actual data (including grid blocks) is current
   * else it will read in the required data from file
   */
  int MakeCurrent();

  void PrintInformation();
  void PrintMemoryUsage();

  //@{
  /**
   * Set and get the current time step to process
   */
  int SetCurrentTime(double time);
  int SetCurrentTimeStep(int timeStep);
  vtkGetMacro(CurrentTime, double);
  vtkGetMacro(CurrentTimeStep, int);
  //@}

  //@{
  /**
   * Set and Get the time range for the simulation run
   */
  vtkGetVector2Macro(TimeStepRange, int);
  vtkGetVector2Macro(TimeRange, double);
  //@}

  //@{
  vtkSetMacro(NeedToCheck, int);
  //@}

  //@{
  /**
   * Functions that map from time to time step and vice versa
   */
  int GetTimeStepFromTime(double time);
  double GetTimeFromTimeStep(int timeStep);
  //@}

  vtkGetMacro(NumberOfCellFields, int);

  double* GetTimeArray();

  /**
   * Returns 1 if the grid information contained in the file has
   * Adaptive Mesh Refinement (AMR) else it returns 0
   */
  int IsAMR();

  /**
   * Return the number of grids in the reader
   */
  int GetNumberOfDataBlocks();

  /**
   * Return the name of the ith field
   */
  const char* GetCellFieldName(int field);

  /**
   * Return the data array of the block's field.  The "fixed"
   * argument is set to 1 if the array has been corrected for
   * bad ghost cells else it is set to 0
   */
  vtkDataArray* GetCellFieldData(int block, int field, int* fixed);

  /**
   * Return the mass data array for the material index passed in
   * for the passed in block
   */
  vtkDataArray* GetMaterialMassField(const int& block, const int& materialIndex);

  /**
   * Return the volume fraction data array for the material index passed in
   * for the passed in block
   */
  vtkDataArray* GetMaterialVolumeFractionField(const int& block, const int& materialIndex);

  /**
   * Mark the block's field to have been fixed w/r bad ghost cells
   */
  int MarkCellFieldDataFixed(int block, int field);

  /**
   * Return the data array for the tracer positions.
   */
  vtkFloatArray* GetTracers();

  // Return the ith block (i.e. grid) in the reader
  vtkSpyPlotBlock* GetBlock(int i);

  // Returns the number of materials
  int GetNumberOfMaterials() const { return NumberOfMaterials; }

  // Returns the number of dimensions
  int GetNumberOfDimensions() const { return NumberOfDimensions; }

  // Returns the coordinate system of the file
  int GetCoordinateSystem() const { return IGM; }

  struct CellMaterialField
  {
    char Id[30];
    char Comment[80];
    int Index;
  };
  struct Variable
  {
    char* Name;
    int Material;
    int Index;
    CellMaterialField* MaterialField;
    vtkDataArray** DataBlocks;
    int* GhostCellsFixed;
  };
  struct DataDump
  {
    int NumVars;
    int* SavedVariables;
    vtkTypeInt64* SavedVariableOffsets;
    vtkTypeInt64 SavedBlocksGeometryOffset;
    unsigned char* SavedBlockAllocatedStates;
    vtkTypeInt64 BlocksOffset;
    Variable* Variables;
    int NumberOfBlocks;
    int ActualNumberOfBlocks;
    int NumberOfTracers;
    vtkFloatArray* TracerCoord;
    vtkIntArray* TracerBlock;
  };
  struct MarkerMaterialField
  {
    char Name[30];
    char Label[256];
  };
  struct MaterialMarker
  {
    int NumMarks;
    int NumRealMarks;
    int NumVars;
    MarkerMaterialField* Variables;
  };
  struct MarkerDump
  {
    vtkFloatArray* XLoc;
    vtkFloatArray* YLoc;
    vtkFloatArray* ZLoc;
    vtkIntArray* ILoc;
    vtkIntArray* JLoc;
    vtkIntArray* KLoc;
    vtkIntArray* Block;
    vtkFloatArray** Variables;
  };
  MaterialMarker* Markers;
  MarkerDump* MarkersDumps;
  vtkSetMacro(GenerateMarkers, int);
  vtkGetMacro(GenerateMarkers, int);

  vtkGetMacro(MarkersOn, int);

  vtkSpyPlotBlock* GetDataBlock(int block);

  vtkSetMacro(DataTypeChanged, int);
  void SetDownConvertVolumeFraction(int vf);

protected:
  vtkSpyPlotUniReader();
  ~vtkSpyPlotUniReader() override;
  vtkSpyPlotBlock* Blocks;

private:
  int RunLengthDataDecode(const unsigned char* in, int inSize, float* out, int outSize);
  int RunLengthDataDecode(const unsigned char* in, int inSize, int* out, int outSize);
  int RunLengthDataDecode(const unsigned char* in, int inSize, unsigned char* out, int outSize);

  int ReadHeader(vtkSpyPlotIStream* spis);
  int ReadMarkerHeader(vtkSpyPlotIStream* spis);
  int ReadCellVariableInfo(vtkSpyPlotIStream* spis);
  int ReadMaterialInfo(vtkSpyPlotIStream* spis);
  int ReadGroupHeaderInformation(vtkSpyPlotIStream* spis);
  int ReadDataDumps(vtkSpyPlotIStream* spis);
  int ReadMarkerDumps(vtkSpyPlotIStream* spis);

  vtkDataArray* GetMaterialField(const int& block, const int& materialIndex, const char* Id);

  // Header information
  char FileDescription[128];
  int FileVersion;
  int SizeOfFilePointer;
  int FileCompressionFlag;
  int FileProcessorId;
  int NumberOfProcessors;
  int IGM;
  int NumberOfDimensions;
  int NumberOfMaterials;
  int MaximumNumberOfMaterials;
  double GlobalMin[3];
  double GlobalMax[3];
  int NumberOfBlocks;
  int MaximumNumberOfLevels;
  int MarkersOn;
  int GenerateMarkers;

  // For storing possible cell/material fields meta data
  int NumberOfPossibleCellFields;
  CellMaterialField* CellFields;
  int NumberOfPossibleMaterialFields;
  CellMaterialField* MaterialFields;

  // Individual dump information
  int NumberOfDataDumps;
  int* DumpCycle;
  double* DumpTime;
  double* DumpDT; // SPCTH 102 (What is this anyway?)
  vtkTypeInt64* DumpOffset;

  DataDump* DataDumps;

  // File name
  char* FileName;

  // Was information read
  int HaveInformation;

  // Current time and time range information
  int CurrentTimeStep;
  // Time step that the geometry represents
  int GeomTimeStep;
  double CurrentTime;
  int TimeStepRange[2];
  double TimeRange[2];

  // Indicates that the reader needs to check its data
  // (Not its geometry) - the reason is that ReadData
  // will be called a lot and there needs to be a way to
  // optimize this
  int NeedToCheck;

  int DataTypeChanged;
  int DownConvertVolumeFraction;

  int NumberOfCellFields;

  vtkDataArraySelection* CellArraySelection;

  Variable* GetCellField(int field);
  int IsVolumeFraction(Variable* var);

private:
  vtkSpyPlotUniReader(const vtkSpyPlotUniReader&) = delete;
  void operator=(const vtkSpyPlotUniReader&) = delete;
};

inline double* vtkSpyPlotUniReader::GetTimeArray()
{
  return this->DumpTime;
}

inline int vtkSpyPlotUniReader::IsAMR()
{
  return (this->NumberOfBlocks > 1);
}

#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotUniReader.h
