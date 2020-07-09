// See license.md for copyright information.
#ifndef vtkParFlowMetaReader_h
#define vtkParFlowMetaReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkParFlowIOModule.h"

#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include "nlohmann/json.hpp"

#include <array>
#include <set>
#include <string>
#include <vector>

class vtkDataSet;
class vtkDoubleArray;

/**\brief Read ParFlow simulation output.
  *
  * Data is output as 2 images (elevation deflection turned off) or an explicit
  * structured grid (subsurface) and image data (surface) (elevation deflection
  * turned on).
  * In each case, the first output is the 3-D subsurface domain and the
  * second output in the 2-D surface domain.
  *
  * Nearly all variables defined on the outputs are cell-centered.
  * However, subsurface fluid velocity (computed on 3 staggered, face-aligned
  * grids) is interpolated to points.
  * When deflecting by elevation, each cell's points are deflected by the cell's
  * elevation (this feature of vtkExplicitStructuredGrid presents discontinuities
  * at cell boundaries).
  *
  * This reader will work in parallel settings via a uniform grid partitioner
  * that redistributes data from the file according to the number of ranks
  * available.
  */
class VTKPARFLOWIO_EXPORT vtkParFlowMetaReader : public vtkDataObjectAlgorithm
{
public:
  using json = nlohmann::json;
  enum Domain
  {
    Subsurface = 0,
    Surface = 1
  };

  vtkTypeMacro(vtkParFlowMetaReader, vtkDataObjectAlgorithm);
  static vtkParFlowMetaReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get the name of the ".pfmetadata" file to read.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  /// Set/get whether the reader should output the subsurface grid.
  vtkGetMacro(EnableSubsurfaceDomain, int);
  vtkSetMacro(EnableSubsurfaceDomain, int);
  vtkBooleanMacro(EnableSubsurfaceDomain, int);

  /// Set/get whether the reader should output the surface grid.
  vtkGetMacro(EnableSurfaceDomain, int);
  vtkSetMacro(EnableSurfaceDomain, int);
  vtkBooleanMacro(EnableSurfaceDomain, int);

  /// Set/get the volume of interest for the subsurface.
  vtkGetVector6Macro(SubsurfaceVOI, int);
  vtkSetVector6Macro(SubsurfaceVOI, int);

  /// Set/get the area of interest for the surface.
  vtkGetVector4Macro(SurfaceAOI, int);
  vtkSetVector4Macro(SurfaceAOI, int);

  /// Set/get whether the reader should deflect the mesh by elevation (if available).
  vtkGetMacro(DeflectTerrain, int);
  vtkSetMacro(DeflectTerrain, int);
  vtkBooleanMacro(DeflectTerrain, int);

  /// Set/get the scale factor applied to the elevation when DeflectTerrain is on.
  vtkGetMacro(DeflectionScale, double);
  vtkSetMacro(DeflectionScale, double);

  /// Set/get the timestep to load in the absence of the UPDATE_TIME_STEP.from the request.
  ///
  /// This defaults to 0.
  vtkGetMacro(TimeStep, int);
  vtkSetMacro(TimeStep, int);

  /// When run in parallel, set/get the number of ghost layers to use.
  vtkSetMacro(NumberOfGhostLayers, int);
  vtkGetMacro(NumberOfGhostLayers, int);

  /// When run in parallel, set/get whether to duplicate nodes.
  vtkSetMacro(DuplicateNodes, int);
  vtkGetMacro(DuplicateNodes, int);

  /// Return the number of input and solution variables defined on the 3D simulation grid.
  int GetNumberOfSubsurfaceVariableArrays() const;
  /// Return the name of the i-th variable defined on the 3D simulation grid.
  std::string GetSubsurfaceVariableArrayName(int idx) const;
  /// Return whether the given \a array is set to be loaded or not.
  int GetSubsurfaceVariableArrayStatus(const std::string& array) const;
  /// Set whether an array defined on the 3D simulation grid should be loaded.
  bool SetSubsurfaceVariableArrayStatus(const std::string& array, int status);

  /// Return the number of input and solution variables defined on the 2D simulation grid.
  int GetNumberOfSurfaceVariableArrays() const;
  /// Return the name of the i-th variable defined on the 2D simulation grid.
  std::string GetSurfaceVariableArrayName(int idx) const;
  /// Return whether the given \a array is set to be loaded or not.
  int GetSurfaceVariableArrayStatus(const std::string& array) const;
  /// Set whether an array defined on the 2D simulation grid should be loaded.
  bool SetSurfaceVariableArrayStatus(const std::string& array, int status);

protected:
  vtkParFlowMetaReader();
  virtual ~vtkParFlowMetaReader();

  /// Set/get the directory holding the .pfmetadata file (for use in relative paths).
  vtkGetStringMacro(Directory);
  vtkSetStringMacro(Directory);

  /// Broadcast a JSON string to all ranks and parse it, returning true on success.
  bool BroadcastMetadata(std::vector<uint8_t>& metadata);

  /// Ingest the JSON metadata, storing which arrays and meshes are available.
  bool IngestMetadata();
  /// Process the given variable or other metadata item named \a key taking on \a value.
  bool IngestMetadataItem(const std::string& section, const std::string& key, const json& value);

  /// Replace (or insert) the current json entry for the given \a key in \a vmap with \a value.
  void ReplaceVariable(
    std::map<std::string, json>& vmap, const std::string& key, const json& value);

  /// Clamp the requested region of interest to the available data.
  template <int dimension>
  void ClampRequestToData(int extentOut[6], const int regionOfInterest[2 * dimension],
    const int availableData[2 * dimension]) const;

  /// Increment the extent size so it reflects point indices rather than cell indices.
  ///
  /// When dimension is 1 or 2, only the the first 2*dimension entries are modified.
  template <int dimension>
  void CellExtentToPointExtent(int extent[6]) const;

  /// Return the closest integer time step in the dataset that is not greater than the provided
  /// time.
  int ClosestTimeStep(double time) const;

  /// Populate the provided multiblock with a distributed mesh that has the given total extent.
  void GenerateDistributedMesh(
    vtkDataSet* data, int extent[6], vtkVector3d& origin, vtkVector3d& spacing);

  /// Find a set of PFB files to load into a possibly vector-valued variable given a timestep.
  ///
  /// On success, \a timestep will contain the actual timestep that the files contain,
  /// which may be different than the requested time (always smaller than the requested
  /// time, except when no prior data exists).
  ///
  /// Also, if the \a slice parameter is -2 on input then it is assumed the files will be
  /// stacked 2-d timeseries slices and on success, \a slice will be set to a non-negative
  /// value indicating which k-slice of the image to read for the given timestep. A \a slice
  /// value of -1 indicates that the data is not 2-d timeseries data stacked into a volume.
  int FindPFBFiles(std::vector<std::string>& filesToLoad, const json& variableMetadata,
    int& timestep, int& slice) const;

  /// Get grid topology on rank 0.
  ///
  /// This sets IJKDivs on rank 0.
  void ScanBlocks(istream& file, int nblocks);

  /// Broadcast grid topology from rank 0.
  ///
  /// This sets IJKDivs on all ranks.
  void BroadcastBlocks();

  /// Use grid topology to compute a block (subgrid) offset.
  ///
  /// Only call this after IJKDivs has been set.
  std::streamoff GetBlockOffset(Domain dom, int blockId, int nz) const;
  std::streamoff GetBlockOffset(Domain dom, const vtkVector3i& blockIJK, int nz) const;

  /// Use current grid topology to compute a just-past-the-end subgrid offset.
  ///
  /// This method can be used to quickly advance to the
  /// subgrid header just past the end of known space.
  std::streamoff GetEndOffset(Domain dom) const;

  static bool ReadSubgridHeader(istream& pfb, vtkVector3i& si, vtkVector3i& sn, vtkVector3i& sr);

  static bool ReadComponentSubgridOverlap(istream& pfb, const vtkVector3i& si,
    const vtkVector3i& sn, const int extent[6], int component, vtkDoubleArray* variable);

  int LoadPFBComponent(Domain dom, vtkDoubleArray* variable, const std::string& filename,
    int component, const int extent[6]) const;

  int LoadPFB(vtkDataSet* data, const int extent[6], const std::vector<std::string>& fileList,
    const std::string& variableName, const json& variableMetadata) const;

  /// Read data from the file(s) described by the variable info into the given grid.
  int LoadVariable(
    vtkDataSet* grid, const std::string& variableName, const json& variableMetadata, int timestep);

  /// Generate a subsurface mesh and add requested variables.
  int LoadSubsurfaceData(vtkDataSet*, int timestep);

  /// Generate a surface mesh and add requested variables.
  int LoadSurfaceData(vtkDataSet*, int timestep);

  /// Update the reader's output dataset types.
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /// Update the reader's output.
  int RequestInformation(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /// Update the reader's output.
  int RequestDataObject(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;
  /*
    */

  /// Update the reader's output.
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /// The filename, which must be a valid path before RequestData is called.
  char* FileName;
  /// The directory containing the filename (so that relative paths in .pfmetadata files work).
  char* Directory;
  int NumberOfGhostLayers;
  int DuplicateNodes;
  /// Whether to output the 3D and/or 2D domains. If both are 0, the reader's output is empty.
  int EnableSubsurfaceDomain;
  int EnableSurfaceDomain;
  /// If enabled, the VOI and AOI specify subregions of interest to generate.
  int SubsurfaceVOI[6]; // subregion of SubsurfaceExtent (below) to fetch
  int SurfaceAOI[4];    // subregion of SurfaceExtent (below) to fetch
  /// Whether to deflect terrain-following grids by elevation.
  int DeflectTerrain;
  double DeflectionScale;
  /// The time step (taken from the set of available TimeSteps) to load.
  int TimeStep;
  /// Cached data to avoid re-reads
  //@{
  /// Timestep used for last read of subsurface data
  int CacheTimeStep;
  /// Cache of subsurface data
  vtkSmartPointer<vtkDataObject> SubsurfaceCache;
  /// Cache of surface data
  vtkSmartPointer<vtkDataObject> SurfaceCache;
  //@}

  /// Metadata and user field-selections for the current ".pfmetadata" file.
  //@{
  /// The entire metadata structure.
  json Metadata;

  /// 3D grid size, spacing, and origin of entire simulation in file.
  int SubsurfaceExtent[6];
  vtkVector3d SubsurfaceOrigin;
  vtkVector3d SubsurfaceSpacing;

  /// 2D grid size, spacing, and origin of entire simulation in file.
  int SurfaceExtent[6];
  vtkVector3d SurfaceOrigin;
  vtkVector3d SurfaceSpacing;

  /// Variables listed in the metadata file.
  std::map<std::string, json> SubsurfaceVariables;
  std::map<std::string, json> SurfaceVariables;

  /// Union of all timesteps present in any variable.
  std::set<int> TimeSteps;

  /// Divider points along each axis for subgrids inside each PFB file,
  /// for each Domain (subsurface and surface).
  ///
  /// This is used inside LoadPFBComponent to determine which subgrids
  /// intersect the region-of-interest and to obtain the offset of each
  /// of those subgrids.
  std::vector<int> IJKDivs[2][3];
  //@}
};

template <int dimension>
void vtkParFlowMetaReader::ClampRequestToData(int extentOut[6],
  const int regionOfInterest[2 * dimension], const int availableData[2 * dimension]) const
{
  for (int axis = 0; axis < dimension; ++axis)
  {
    extentOut[2 * axis] = regionOfInterest[2 * axis] < availableData[2 * axis]
      ? availableData[2 * axis]
      : regionOfInterest[2 * axis] > availableData[2 * axis + 1] ? availableData[2 * axis + 1]
                                                                 : regionOfInterest[2 * axis];
    extentOut[2 * axis + 1] = regionOfInterest[2 * axis + 1] > availableData[2 * axis + 1] ||
        regionOfInterest[2 * axis + 1] < 0
      ? availableData[2 * axis + 1]
      : regionOfInterest[2 * axis + 1] < availableData[2 * axis] ? availableData[2 * axis]
                                                                 : regionOfInterest[2 * axis + 1];
  }
  for (int axis = dimension; axis < 3; ++axis)
  {
    extentOut[2 * axis] = 0;
    extentOut[2 * axis + 1] = 0;
  }
}

template <int dimension>
void vtkParFlowMetaReader::CellExtentToPointExtent(int extent[6]) const
{
  for (int axis = 0; axis < (dimension <= 3 ? dimension : 3); ++axis)
  {
    ++extent[2 * axis + 1];
  }
}

#endif // vtkParFlowMetaReader_h
