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

#include "vtkCGNSCache.h" // for vtkCGNSCache, caching of mesh and connectivity
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h"                             // for vtkNew.
#include "vtkPVVTKExtensionsCGNSReaderModule.h" // for export macro

class vtkDataSet;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkCGNSSubsetInclusionLattice;
class vtkPoints;
class vtkUnstructuredGrid;

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  //@{
  /**
   * Convenience API to query information about bases and enable/disable loading
   * of bases. One can also get the sil (`vtkCGNSReader::GetSIL()`) and then use
   * API on vtkCGNSSubsetInclusionLattice to enable/disable blocks with
   * additional flexibility.
   */
  int GetBaseArrayStatus(const char* name);
  void SetBaseArrayStatus(const char* name, int status);
  void DisableAllBases();
  void EnableAllBases();
  int GetNumberOfBaseArrays();
  const char* GetBaseArrayName(int index);
  //@}

  //@{
  /**
   * Convenience API to query information about families and enable/disable loading
   * of families. One can also get the sil (`vtkCGNSReader::GetSIL()`) and then use
   * API on vtkCGNSSubsetInclusionLattice to enable/disable blocks with
   * additional flexibility.
   */
  int GetNumberOfFamilyArrays();
  const char* GetFamilyArrayName(int index);
  void SetFamilyArrayStatus(const char* name, int status);
  int GetFamilyArrayStatus(const char* name);
  void EnableAllFamilies();
  void DisableAllFamilies();
  //@}

  /**
   * Provides access to the SIL. The SIL is populated in `RequestInformation`.
   * Once populated, one can use API on vtkCGNSSubsetInclusionLattice to query
   * information about bases/zones/families etc. and enable/disable loading of
   * those.
   */
  vtkCGNSSubsetInclusionLattice* GetSIL() const;

  //@{
  /**
   * API to select blocks to read. A more expressive API is provided by
   * vtkCGNSSubsetInclusionLattice which can be access using `GetSIL` method.
   * These methods provide some rudimentary API on the reader itself.
   */
  void SetBlockStatus(const char* nodepath, bool enable);
  void ClearBlockStatus();
  //@}

  //@{
  /**
   * API to get information of point arrays and enable/disable loading of
   * a particular arrays.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  //@}

  //@{
  /**
   * API to get information of cell arrays and enable/disable loading of
   * a particular arrays.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  //@}

  vtkSetMacro(DoublePrecisionMesh, int);
  vtkGetMacro(DoublePrecisionMesh, int);
  vtkBooleanMacro(DoublePrecisionMesh, int);

  //@{
  /**
   * Enable/disable loading of boundary condition patches.
   * Defaults to off.
   * @deprecated Use SIL instead.
   */
  VTK_LEGACY(void SetLoadBndPatch(int));
  VTK_LEGACY(vtkGetMacro(LoadBndPatch, int));
  VTK_LEGACY(void LoadBndPatchOn());
  VTK_LEGACY(void LoadBndPatchOff());
  //@}

  //@{
  /**
   * Enable/disable loading of zone mesh. Defaults to on. It may be turned off
   * to load only boundary patches (when LoadBndPatch if ON), for example.
   * @deprecated Use SIL instead.
   */
  VTK_LEGACY(void SetLoadMesh(bool));
  VTK_LEGACY(vtkGetMacro(LoadMesh, bool));
  VTK_LEGACY(void LoadMeshOn());
  VTK_LEGACY(void LoadMeshOff());
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
   * This reader can cache the mesh points if they are time invariant.
   * They will be stored with a unique reference to their /base/zonename
   * and not be read in the file when doing unsteady analysis.
   */
  void SetCacheMesh(bool enable);
  vtkGetMacro(CacheMesh, bool);
  vtkBooleanMacro(CacheMesh, bool);

  //@{
  /**
   * This reader can cache the meshconnectivities if they are time invariant.
   * They will be stored with a unique reference to their /base/zonename
   * and not be read in the file when doing unsteady analysis.
   */
  void SetCacheConnectivity(bool enable);
  vtkGetMacro(CacheConnectivity, bool);
  vtkBooleanMacro(CacheConnectivity, bool);

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

  /**
   * Return the timestamp for the rebuilding of the SIL.
   */
  vtkIdType GetSILUpdateStamp() const;

protected:
  vtkCGNSReader();
  ~vtkCGNSReader() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;

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
  vtkCGNSReader(const vtkCGNSReader&) = delete;
  void operator=(const vtkCGNSReader&) = delete;

  /**
   * callback called when SIL selection is modified.
   */
  void OnSILStateChanged();
  bool IgnoreSILChangeEvents;

  CGNSRead::vtkCGNSMetaData* Internal;               // Metadata
  CGNSRead::vtkCGNSCache<vtkPoints> MeshPointsCache; // Cache for the mesh points
  CGNSRead::vtkCGNSCache<vtkUnstructuredGrid>
    ConnectivitiesCache; // Cache for the mesh connectivities

  char* FileName; // cgns file name
#if !defined(VTK_LEGACY_REMOVE)
  int LoadBndPatch; // option to set section loading for unstructured grid
  bool LoadMesh;    // option to enable/disable mesh loading
#endif
  int DoublePrecisionMesh;       // option to set mesh loading to double precision
  int CreateEachSolutionAsBlock; // debug option to create
  bool IgnoreFlowSolutionPointers;
  bool DistributeBlocks;
  bool CacheMesh;
  bool CacheConnectivity;

  // For internal cgio calls (low level IO)
  int cgioNum;      // cgio file reference
  double rootId;    // id of root node
  double currentId; // id of node currently being read (zone)
  //
  unsigned int NumberOfBases;
  int ActualTimeStep;

  class vtkPrivate;
  friend class vtkPrivate;

  /**
   * When reading a temporal file series, we don't want to rebuild the SIL for
   * each file since it doesn't change (or is not expected to change).
   *
   * When reading a partitioned file series, we may not have full information in
   * each file to fully build the SIL. The vtkCGNSFileSeriesReader handles
   * building of the SIL across ranks/files and wouldn't want the reader to
   * update the SIL every time the filename changes, which happens may times for
   * file series. To support that, we allow vtkCGNSFileSeriesReader to simply
   * set to SIL. If set, we don't update it on each "parse" of a new file.
   */
  friend class vtkCGNSFileSeriesReader;
  void SetExternalSIL(vtkCGNSSubsetInclusionLattice* sil);
};

#endif // vtkCGNSReader_h
