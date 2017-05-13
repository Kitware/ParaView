/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSReader.h

  Copyright (c) Ken Martin, Will Schrodeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// Copyright 2013-2014 Mickael Philit.

/**
 * @class   vtkCGNSReader
 *
 * vtkCGNSReader creates a multi-block dataset and reads unstructured grids,
 * and structured meshes from binary files stored in CGNS file format,
 * with data stored at the nodes or at the cells.
 *
 * vtkCGNSReader is inspired by the VisIt CGNS reader originally written by
 * B. Whitlock. vtkCGNSReader relies on the low level CGNS API to load DataSet
 * and reduce memory footprint.
 *
 * @warning
 *   ...
 *
 * @par Thanks:
 * Thanks to .
*/

#ifndef vtkCGNSReader_h
#define vtkCGNSReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h"                             // for vtkNew.
#include "vtkPVVTKExtensionsCGNSReaderModule.h" // for export macro

class vtkDataSet;
class vtkDataArraySelection;
class vtkCallbackCommand;

namespace CGNSRead
{
class vtkCGNSMetaData;
}

class vtkMultiProcessController;
class VTKPVVTKEXTENSIONSCGNSREADER_EXPORT vtkCGNSReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCGNSReader* New();
  vtkTypeMacro(vtkCGNSReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name of CGNS datafile to read
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Is the given file name a CGNS file?
   */
  int CanReadFile(const char* filename);

  // The following methods allow selective reading of solutions fields.
  int GetBaseArrayStatus(const char* name);
  void SetBaseArrayStatus(const char* name, int status);
  void DisableAllBases();
  void EnableAllBases();
  int GetNumberOfBaseArrays();

  //@{
  /**
   * Method that allows setting/getting of node families.
   */
  int GetNumberOfFamilyArrays();
  const char* GetFamilyArrayName(int index);
  void SetFamilyArrayStatus(const char* name, int status);
  int GetFamilyArrayStatus(const char* name);
  void EnableAllFamilies();
  void DisableAllFamilies();
  //@}

  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();

  const char* GetBaseArrayName(int index);
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);

  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);

  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);

  void DisableAllPointArrays();
  void EnableAllPointArrays();

  void DisableAllCellArrays();
  void EnableAllCellArrays();

  vtkSetMacro(DoublePrecisionMesh, int);
  vtkGetMacro(DoublePrecisionMesh, int);
  vtkBooleanMacro(DoublePrecisionMesh, int);

  //@{
  /**
   * Enable/disable loading of boundary condition patches.
   * Defaults to off.
   */
  vtkSetMacro(LoadBndPatch, int);
  vtkGetMacro(LoadBndPatch, int);
  vtkBooleanMacro(LoadBndPatch, int);
  //@}

  //@{
  /**
   * Enable/disable loading of zone mesh. Defaults to on. It may be turned off
   * to load only boundary patches (when LoadBndPatch if ON), for example.
   */
  vtkSetMacro(LoadMesh, bool);
  vtkGetMacro(LoadMesh, bool);
  vtkBooleanMacro(LoadMesh, bool);
  //@}

  /**
   * This option is provided for debugging and should not be used for production
   * runs as the output data produced may not be correct. When set to true, the
   * read will simply read each solution (`FlowSolution_t`) node encountered in
   * a zone and create a separate block under the block corresponding to the
   * zone in the output.
   */
  vtkSetMacro(CreateEachSolutionAsBlock, int);
  vtkGetMacro(CreateEachSolutionAsBlock, int);
  vtkBooleanMacro(CreateEachSolutionAsBlock, int);

  /**
   * When set to true (default is false), the reader will simply
   * ignore `FlowSolutionPointers` since they are either incomplete or invalid
   * and instead will rely on FlowSolution_t nodes being labelled as
   * "...AtStep<tsindex>" to locate solution nodes for a specific timestep.
   * Note, tsindex starts with 1 (not zero).
   *
   * When set to false, the reader will still try to confirm that at least one
   * valid FlowSolution_t node is referred to in FlowSolutionPointers nodes for the
   * current timestep. If none is found, then the reader will print out a
   * warning and act as if IgnoreFlowSolutionPointers was set to true. To avoid
   * this warning, one should set IgnoreFlowSolutionPointers to true.
   */
  vtkSetMacro(IgnoreFlowSolutionPointers, bool);
  vtkGetMacro(IgnoreFlowSolutionPointers, bool);
  vtkBooleanMacro(IgnoreFlowSolutionPointers, bool);

  /**
   * This reader can support piece requests by distributing each block in each
   * zone across ranks (default). To make the reader disregard piece request and
   * read all blocks in the zone, set this to false (default is true).
   */
  vtkSetMacro(DistributeBlocks, bool);
  vtkGetMacro(DistributeBlocks, bool);
  vtkBooleanMacro(DistributeBlocks, bool);

  //@{
  /**
   * Set/get the communication object used to relay a list of files
   * from the rank 0 process to all others. This is the only interprocess
   * communication required by vtkPExodusIIReader.
   */
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Sends metadata (that read from the input file, not settings modified
   * through this API) from the rank 0 node to all other processes in a job.
   */
  void Broadcast(vtkMultiProcessController* ctrl);

protected:
  vtkCGNSReader();
  ~vtkCGNSReader();

  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  vtkNew<vtkDataArraySelection> BaseSelection;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> FamilySelection;

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  int GetCurvilinearZone(
    int base, int zone, int cell_dim, int phys_dim, void* zsize, vtkMultiBlockDataSet* mbase);

  int GetUnstructuredZone(
    int base, int zone, int cell_dim, int phys_dim, void* zsize, vtkMultiBlockDataSet* mbase);
  vtkMultiProcessController* Controller;
  vtkIdType ProcRank;
  vtkIdType ProcSize;

private:
  vtkCGNSReader(const vtkCGNSReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCGNSReader&) VTK_DELETE_FUNCTION;

  CGNSRead::vtkCGNSMetaData* Internal; // Metadata

  char* FileName;                // cgns file name
  int LoadBndPatch;              // option to set section loading for unstructured grid
  bool LoadMesh;                 // option to enable/disable mesh loading
  int DoublePrecisionMesh;       // option to set mesh loading to double precision
  int CreateEachSolutionAsBlock; // debug option to create
  bool IgnoreFlowSolutionPointers;
  bool DistributeBlocks;

  // For internal cgio calls (low level IO)
  int cgioNum;      // cgio file reference
  double rootId;    // id of root node
  double currentId; // id of node currently being read (zone)
  //
  unsigned int NumberOfBases;
  int ActualTimeStep;

  class vtkPrivate;
  friend class vtkPrivate;
};

#endif // vtkCGNSReader_h
