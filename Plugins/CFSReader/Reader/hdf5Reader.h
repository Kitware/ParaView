// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   hdf5Reader
 * @brief   holds a hdf5 based openCFS file and serves logical features to vtkCFSReader
 */

#ifndef HDF5READER_H
#define HDF5READER_H

#include <vector>

#include <vtkSystemIncludes.h>
#include <vtk_hdf5.h>

#include "hdf5Common.h" // same directory

namespace H5CFS
{

/**
 * Class for reading in mesh and simulation data from hdf5 file
 *
 * Class for handling the reading of mesh and simulation data from
 * HDF5 files.
 */
class VTK_EXPORT Hdf5Reader
{
public:
  Hdf5Reader() = default;

  virtual ~Hdf5Reader();

  /**
   * Initialize module
   */
  void LoadFile(const std::string& fileName);

  /**
   * Close H5 file object and the H5 main group
   */
  void CloseFile();

  /**
   * Return dimension of the mesh (2 or 3)
   */
  unsigned int GetDimension() const;

  /**
   * Return the gridOrder (1 for linear)
   */
  unsigned int GetGridOrder() const;

  /**
   * Get all nodal coordinates
   */
  void GetNodeCoordinates(std::vector<std::vector<double>>& coords) const;

  /**
   * Get all element definitions
   */
  void GetElements(
    std::vector<ElemType>& type, std::vector<std::vector<unsigned int>>& connect) const;

  /**
   * Get elements of specific region
   */
  const std::vector<unsigned int>& GetElementsOfRegion(const std::string& regionName);

  /**
   * Get nodes of specific region
   */
  const std::vector<unsigned int>& GetNodesOfRegion(const std::string& regionName);

  /**
   * Get nodes of named node group
   */
  const std::vector<unsigned int>& GetNamedNodes(const std::string& name);

  /**
   * Get elems of named elem group
   */
  const std::vector<unsigned int>& GetNamedElements(const std::string& name);

  /**
   * Get vector with all region names in mesh

   * Returns a vector with the names of regions in the mesh of all
   * dimensions.
   * \note Since the regionIdType is guaranteed to be defined by
   * a number type (unsigned int, unsigned int32), the regionId of an element can
   * be directly used as index to the regions-vector.
   */
  const std::vector<std::string>& GetAllRegionNames() { return RegionNames; }

  /**
   * Returns a vector which contains all names of named nodes.
   */
  const std::vector<std::string>& GetNodeNames() { return NodeNames; }

  /**
   * Returns a vector which contains all names of element groups
   */
  const std::vector<std::string>& GetElementNames() { return ElementNames; }

  /**
   * Get entities (nodes, elements), on which a result is defined on
   */
  const std::vector<unsigned int>& GetEntities(EntityType type, const std::string& name);

  /**
   * Return multisequence steps and their analysis types
   */
  void GetNumberOfMultiSequenceSteps(std::map<unsigned int, AnalysisType>& analysis,
    std::map<unsigned int, unsigned int>& numSteps, bool isHistory = false) const;

  /**
   * Obtain list with result types in each sequence step
   */
  void GetResultTypes(unsigned int sequenceStep, std::vector<std::shared_ptr<ResultInfo>>& infos,
    bool isHistory = false) const;

  /**
   * Return list with time / frequency values and step for a given result
   */
  void GetStepValues(unsigned int sequenceStep, const std::string& infoName,
    std::map<unsigned int, double>& steps, bool isHistory = false) const;

  /**
   * Fill pre-initialized result object with mesh result of specified step
   */
  void GetMeshResult(unsigned int sequenceStep, unsigned int stepNum, Result* result);

  /**
   * Fill pre-initialized result object with history result of specified entity
   * e.g. /Results/History/MultiStep_1/mechTotalEnergy/Regions/mech
   * @param entityId is the region string
   * @param result resultInfo gives name (mechTotalEnergy) and realVal and imaginaryVal are set
   * */
  void GetHistoryResult(
    unsigned int sequenceStep, const std::string& entityId, Result* result) const;

private:
  /**
   * Setup mesh information like regions and named entities
   */
  void ReadMeshStatusInformations();

  /**
   * File property obtained by H5Pcreate() before opening MainFile via H5Fopen()
   */
  hid_t FileProperties = -1;

  /**
   * Main hdf5 file, HDF-C-API object type
   */
  hid_t MainFile = -1;

  /**
   * Root group of main file
   */
  hid_t MainRoot = -1;

  /**
   * Root group for mesh section
   */
  hid_t MeshRoot = -1;

  std::string FileName;

  /**
   * Native directory path to hdf5 file
   */
  std::string BaseDir;

  /**
   * Flag indicating use of external files for mesh results
   */
  bool HasExternalFiles = false;

  std::vector<std::string> RegionNames;

  /**
   * Map with number of dimensions for each region
   * Surface regions have lower dimensions than volume regions.
   */
  std::map<std::string, unsigned int> RegionDims;

  /**
   * Map with element numbers for each region
   */
  std::map<std::string, std::vector<unsigned int>> RegionElems;

  /**
   * Map with node numbers for each region
   */
  std::map<std::string, std::vector<unsigned int>> RegionNodes;

  /**
   * List with names of node groups
   */
  std::vector<std::string> NodeNames;

  /**
   * List with names of element groups
   */
  std::vector<std::string> ElementNames;

  /**
   * Map with element numbers for each named element group
   */
  std::map<std::string, std::vector<unsigned int>> EntityElems;

  /**
   * Map with element numbers for each named element and node group
   */
  std::map<std::string, std::vector<unsigned int>> EntityNodes;

  unsigned int NumNodes = 0;

  unsigned int NumElems = 0;

  std::vector<double> NodeCoords;
}; // end of class

} // end of namespace H5CFS

#endif // HDF5COMMON_H
