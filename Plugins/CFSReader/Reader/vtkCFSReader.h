// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCFSReader
 *
 * @brief   integration of reader plugin to read htd5 based openCFS output
 *
 * The file and the logic itself are within hdf5Reader.
 */

#ifndef VTKCFSREADER_H
#define VTKCFSREADER_H

#include <vector>
#include <vtkCellType.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkInformationIntegerVectorKey.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationStringVectorKey.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkObjectBase.h>
#include <vtkSetGet.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkWin32Header.h>

#include "hdf5Reader.h" // within the same directory

#include <vtkCFSReaderModule.h>

class vtkDoubleArray;
class vtkCell;

class VTKCFSREADER_EXPORT vtkCFSReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCFSReader* New();
  vtkTypeMacro(vtkCFSReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Check, if the current file is really a CFS hdf5 file.
   * This method gets called in order to determine the "right" reader in case
   * several readers can open files with the same extension.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* fname);

  /**
   * Specify the file name of the CFS data file to read.
   * Default is empty filename (nullptr)
   */
  virtual void SetFileName(VTK_FILEPATH const char* arg);

  /**
   * Obtain the CFS data file name.
   */
  virtual const char* GetFileName();

  /**
   * Close then file handle
   */
  virtual void CloseFile();

  /**
   * Get the multisequence range. Filled during RequestInformation.
   */
  vtkGetVector2Macro(MultiSequenceRange, int);

  ///@{
  /**
   * Set/Get the current multisequence step.
   * Default is step number 1.
   */
  void SetMultiSequenceStep(int step);
  vtkGetMacro(MultiSequenceStep, int);
  ///@}

  ///@{
  /**
   * Set/Get the current timestep.
   * Default is the timestep 1.
   */
  virtual void SetTimeStep(unsigned int step);
  vtkGetMacro(TimeStep, unsigned int);
  ///@}

  ///@{
  /**
   * Get current frequency / time value
   */
  virtual const char* GetTimeOrFrequencyValueStr();
  ///@}

  ///@{
  /**
   * Add dimensions to array names.
   * Default is 0 for not to extend array names.
   */
  virtual void SetAddDimensionsToArrayNames(int);
  vtkGetMacro(AddDimensionsToArrayNames, int);
  vtkBooleanMacro(AddDimensionsToArrayNames, int);
  ///@}

  ///@{
  /**
   * Form displacement data from complex values.
   * Default is 0 for not adding mode shape displacements.
   */
  virtual void SetHarmonicDataAsModeShape(int);
  vtkGetMacro(HarmonicDataAsModeShape, int);
  vtkBooleanMacro(HarmonicDataAsModeShape, int);
  ///@}

  /**
   * Return the number of time / frequency steps.
   */
  virtual unsigned int GetNumberOfSteps() { return this->NumberOfTimeSteps; }

  ///@{
  /**
   * Get the timestep range. Filled during RequestInformation.
   */
  vtkGetVector2Macro(TimeStepNumberRange, int);

  virtual double* GetTimeStepRange()
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning TimeStepRange pointer "
                  << this->TimeStepValuesRange);

    return static_cast<double*>(this->TimeStepValuesRange);
  }

  virtual void GetTimeStepRange(double& arg1, double& arg2)
  {
    arg1 = this->TimeStepValuesRange[0];
    arg2 = this->TimeStepValuesRange[1];
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning TimeStepRange = ("
                  << arg1 << "," << arg2 << ")");
  };

  virtual void GetTimeStepRange(double arg[2]) { this->GetTimeStepRange(arg[0], arg[1]); }
  ///@}

  ///@{
  /**
   * Type of complex result treating for adding
   * pair real/imaginary or pair amplitude/phase
   * Default is 0 for all cases (data not added)
   */
  void SetComplexReal(int flag)
  {
    this->ComplexModeReal = flag;
    // In addition trigger resetting the data value arrays
    this->ResetDataArrays = true;
    // update pipeline
    this->Modified();
  }
  vtkGetMacro(ComplexModeReal, int);

  /**
   * Add imaginary part of complex data as value set.
   * Default is 0 (off)
   */
  void SetComplexImaginary(int flag)
  {
    this->ComplexModeImaginary = flag;
    // In addition trigger resetting the data value arrays
    this->ResetDataArrays = true;
    // update pipeline
    this->Modified();
  }
  vtkGetMacro(ComplexModeImaginary, int);

  /**
   * Add normed value complex data as value set.
   * Default is 0 (off)
   */
  void SetComplexAmplitude(int flag)
  {
    this->ComplexModeAmplitude = flag;
    // In addition trigger resetting the data value arrays
    this->ResetDataArrays = true;
    // update pipeline
    this->Modified();
  }
  vtkGetMacro(ComplexModeAmplitude, int);

  /**
   * Add phase in read for value complex data as value set.
   * Default is 0 (off)
   */
  void SetComplexPhase(int flag)
  {
    this->ComplexModePhase = flag;
    // In addition trigger resetting the data value arrays
    this->ResetDataArrays = true;
    // update pipeline
    this->Modified();
  }
  vtkGetMacro(ComplexModePhase, int);
  ///@}

  ///@{
  /**
   * Switch if missing results should get filled with 0.
   * 0 = omit empty regions (only partial results available).
   * 1 = fill empty missing results with 0-valued vector.
   * Default is 0 (not filling missing regions).
   */
  void SetFillMissingResults(int);
  vtkGetMacro(FillMissingResults, int);
  vtkBooleanMacro(FillMissingResults, int);
  ///@}

  /**
   * Return spatial dimension of grid
   */
  int GetGridDimension() const { return Dimension; }

  /**
   * Return order of FEM ansatz functions within grid
   */
  int GetGridOrder() const { return GridOrder; }

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfRegionArrays() const { return static_cast<int>(RegionNames.size()); }

  ///@{
  /** Get/Set whether the cell array with the given name is to
   * be read.
   * Default is 1 (to be read)
   */
  int GetRegionArrayStatus(const char* name);
  void SetRegionArrayStatus(const char* name, int status);
  ///@}

  /**
   * Get the name of the  cell array with the given index in
   * the input.
   */
  const char* GetRegionArrayName(int index);

  /**
   * Return the total number of named node arrays within mesh
   */
  int GetNumberOfNamedNodeArrays() const { return static_cast<int>(NamedNodeNames.size()); }

  ///@{
  /**
   * Get/Set whether the named node array with the given name is to
   * be read.
   * Default is 0 (off).
   */
  int GetNamedNodeArrayStatus(const char* name);
  void SetNamedNodeArrayStatus(const char* name, int status);
  ///@}

  /**
   * Return named node array by index.
   */
  const char* GetNamedNodeArrayName(int index);

  /**
   * Return total number of named element arrays within mesh.
   */
  int GetNumberOfNamedElementArrays() const { return static_cast<int>(NamedElementNames.size()); }

  ///@{
  /**
   * Get/Set whether the named elem array with the given name is to
   * be read.
   * Default is 0 (off).
   */
  int GetNamedElementArrayStatus(const char* name);
  void SetNamedElementArrayStatus(const char* name, int status);
  ///@}

  /**
   * Return name of element array by index
   */
  const char* GetNamedElementArrayName(int index);

  /**
   * Get the coordinates of all nodes in the grid
   */
  void GetNodeCoordinates(vtkDoubleArray* coords);

  /**
   * Name of the history result
   */
  static vtkInformationStringKey* CFS_RESULT_NAME();

  /**
   * Vector of Dof names
   */
  static vtkInformationStringVectorKey* CFS_DOF_NAMES();

  /**
   * Determines on which entity the result is defined
   *  1: Node
   *  2: Edge (unused)
   *  3: Face (unused)
   *  4: Element
   *  5: Surface element
   *  6: PFEM (unused)
   *  7: Region
   *  8: Surface region
   *  9: Node group
   * 10: Coil
   * 11: Free/Unknown
   */
  static vtkInformationIntegerKey* CFS_DEFINED_ON();

  /**
   * Name of the entity on which the history result is defined
   */
  static vtkInformationStringKey* CFS_ENTITY_NAME();

  /**
   * Numerical type of the history result
   *  0: Unknown (should not occur)
   *  1: Scalar
   *  3: Vector
   *  6: Tensor
   * 32: String (currently unused)
   */
  static vtkInformationIntegerKey* CFS_ENTRY_TYPE();

  /**
   * Vector of the step numbers at which the history result was saved
   */
  static vtkInformationIntegerVectorKey* CFS_STEP_NUMS();

  /**
   * Vector of the step values at which the history result was saved
   */
  static vtkInformationDoubleVectorKey* CFS_STEP_VALUES();

  /**
   * Unit of the result quantity
   */
  static vtkInformationStringKey* CFS_UNIT();

  /**
   * Vector of the entity IDs (strings) on which the history result is defined
   */
  static vtkInformationStringVectorKey* CFS_ENTITY_IDS();

  /**
   * Index of the multisequence step
   */
  static vtkInformationIntegerKey* CFS_MULTI_SEQ_INDEX();

  /**
   * Type of the analysis performed in the multisequence step
   * 1: static
   * 2: transient
   * 3: harmonic
   * 4: eigenfrequency
   */
  static vtkInformationIntegerKey* CFS_ANALYSIS_TYPE();

protected:
  vtkCFSReader();
  ~vtkCFSReader() override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  /**
   * This object encapsulates the openCFS hf5 file with some service functions
   */
  H5CFS::Hdf5Reader Reader;

  /**
   * name of current file or empty
   */
  std::string FileName;

  /**
   * map cfs/hdf5 element type to vtk native ones
   */
  static VTKCellType GetCellIdType(H5CFS::ElemType type);

  static vtkDoubleArray* SaveToArray(const std::vector<double>& vals,
    const std::vector<std::string>& dofNames, unsigned int numEntities, const std::string& name);

  /**
   * dimension of the grid (2 or 3)
   */
  int Dimension = 0;

  /**
   * order of grid (linear case 1)
   */
  int GridOrder = 0;

  /**
   * The region names within mesh
   */
  std::vector<std::string> RegionNames;

  /**
   * The groups of named elements within mesh
   */
  std::vector<std::string> NamedElementNames;

  /**
   * The groups of named nodes within mesh
   */
  std::vector<std::string> NamedNodeNames;

  /**
   * map (region, globalNodeNumber)->(regionLocalNodeNumber)
   */
  std::vector<std::vector<unsigned int>> NodeMap;

  /**
   * time steps of current multisequence step
   */
  std::vector<double> StepVals;

  /**
   * vector with step values having a result
   */
  std::vector<unsigned int> StepNumbers;

  /**
   * multiblock dataset, which contains for each
   * region an initialized grid
   */
  vtkMultiBlockDataSet* MBDataSet = nullptr;

  /**
   * reduced multiblock dataset, which contains
   * only blocks for currently active regions
   */
  vtkMultiBlockDataSet* MBActiveDataSet = nullptr;

  /**
   * flag array indicating active regions
   */
  std::map<std::string, int> RegionSwitch;

  /**
   * flag array indicating named nodes to read
   */
  std::map<std::string, int> NamedNodeSwitch;

  /**
   * flag array indicating named elems to read
   */
  std::map<std::string, int> NamedElementSwitch;

  /**
   * flag indicating change of multisequence step
   */
  bool MSStepChanged = false;

  /**
   * Analysis type of multi-sequence steps with mesh results
   */
  std::map<unsigned int, H5CFS::AnalysisType> ResAnalysisType;

  /**
   * ResultInfo of mesh results per multi-sequence step
   */
  std::map<unsigned int, std::vector<std::shared_ptr<H5CFS::ResultInfo>>> MeshResultInfos;

  /**
   * Analysis type of multi-sequence step with history results
   */
  std::map<unsigned int, H5CFS::AnalysisType> HistAnalysisType;

  /**
   * ResultInfo of history results per multi-sequence step
   */
  std::map<unsigned int, std::vector<std::shared_ptr<H5CFS::ResultInfo>>> HistResultInfos;

  /**
   * Global steps per multisequence where ANY result is available
   */
  std::map<unsigned int, std::set<std::pair<unsigned int, double>>> GlobalResultSteps;

  /**
   * Current multisequence step, exposed by vtk macro
   */
  unsigned int MultiSequenceStep = 1;

  /**
   * Analysis type of current multisequence step
   */
  H5CFS::AnalysisType AnalysisType = H5CFS::NO_ANALYSIS_TYPE;

  /**
   * Current time step (for discrete time steps), exposed by vtk macro
   */
  unsigned int TimeStep = 1;

  /**
   * Current time step / frequency value pretty printed
   */
  double TimeOrFrequencyValue = 0.0;

  /**
   * pretty formatted time value / frequency value
   */
  std::string TimeOrFrequencyValueStr = "0.0";

  /**
   * The time value which the pipeline requests
   */
  double RequestedTimeValue = 0.0;

  ///@{
  /**
   * The complex mode comes in real/imaginary or amplitude/phase pairs.
   *
   * Exposed by vtk macros.
   */
  unsigned int ComplexModeReal = 0;
  unsigned int ComplexModeImaginary = 0;
  unsigned int ComplexModeAmplitude = 0;
  unsigned int ComplexModePhase = 0;
  ///@}

  /**
   * Switch if missing results should get filled with 0
   * 0 = omit empty regions (only partial results available)
   * 1 = fill empty missing results with 0-valued vector
   * Exposed by vtk macro
   */
  unsigned int FillMissingResults = 0;

  /**
   * Add dimensions to array names. Exposed by vtk macro
   */
  int AddDimensionsToArrayNames = 0;

  /**
   * Interpret harmonic data as transient mode shape.
   * Exposed by vtk macro
   */
  int HarmonicDataAsModeShape = 0;

  /**
   * Are we dealing with harmonic data?
   */
  bool HarmonicData = false;

  /**
   * number of time/freq steps in current ms step
   */
  unsigned int NumberOfTimeSteps = 0;

  /**
   * array with first/last time/freq step value
   */
  int TimeStepNumberRange[2] = { 1,
    1 }; // as this is used in vtk macros, a std::array<> does not work

  /**
   * array with first/last time/freq step value
   */
  double TimeStepValuesRange[2] = { 0.0, 1.0 };

  /**
   *  array with first/last ms step number
   */
  int MultiSequenceRange[2] = { 1, 1 };

  /**
   * flag if hdf5 file is already read in
   */
  bool IsInitialized = false;

  /**
   * flag if hdf5 infos (region names, named node/elems) have been read in
   */
  bool Hdf5InfoRead = false;

  /**
   * flag, if region settings were modified
   */
  bool ActiveRegionsChanged = false;

  /**
   * flag, to reset the data value arrays (e.g. after
   * switching the complex mode etc.)
   */
  bool ResetDataArrays = false;

  /**
   * Actually opens the file and reads region and group names
   */
  void ReadHdf5Informations();

  void ReadFile(vtkMultiBlockDataSet* output);
  void ReadNodes(vtkMultiBlockDataSet* output);
  void ReadCells(vtkMultiBlockDataSet* output);

  /**
   * Create single grid from elements of one region / group
   */
  void AddElements(vtkUnstructuredGrid* actGrid, unsigned int blockIndex,
    const std::vector<unsigned int>& elems, std::vector<H5CFS::ElemType>& types,
    std::vector<std::vector<unsigned int>>& connect);

  void ReadNodeCellData(vtkMultiBlockDataSet* output, bool isNode);

  /**
   * Update multiblock dataset according to current region/block settings
   */
  void UpdateActiveRegions();

  vtkCFSReader(const vtkCFSReader&) = delete;   // Not implemented.
  void operator=(const vtkCFSReader&) = delete; // Not implemented.
};

#endif // VTKCFSREADER_H
