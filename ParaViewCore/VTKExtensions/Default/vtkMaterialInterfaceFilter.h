/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMaterialInterfaceFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMaterialInterfaceFilter
 * @brief   Extract particles and analyse them.
 *
 * This filter takes a cell data volume fraction and generates a polydata
 * surface.  It also performs connectivity on the particles and generates
 * a particle index as part of the cell data of the output.  It computes
 * the volume of each particle from the volume fraction.
 *
 * This will turn on validation and debug i/o of the filter.
 * \code{.cpp}
 * #define vtkMaterialInterfaceFilterDEBUG
 * \endcode
 *
 * This will turn on profiling of how long each part of the filter takes
 * \code{.cpp}
 * #define vtkMaterialInterfaceFilterPROFILE
 * \endcode
*/

#ifndef vtkMaterialInterfaceFilter_h
#define vtkMaterialInterfaceFilter_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include <string>                            // needed for string
#include <vector>                            // needed for vector

#include "vtkSmartPointer.h" // needed for smart pointer
#include "vtkTimerLog.h"     // needed for vtkTimerLog.

class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkNonOverlappingAMR;
class vtkPoints;
class vtkDoubleArray;
class vtkCellArray;
class vtkCellData;
class vtkIntArray;
class vtkMultiProcessController;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkImplicitFunction;

// specific to us
class vtkMaterialInterfaceLevel;
class vtkMaterialInterfaceFilterBlock;
class vtkMaterialInterfaceFilterIterator;
class vtkMaterialInterfaceEquivalenceSet;
class vtkMaterialInterfaceFilterRingBuffer;
class vtkMaterialInterfacePieceLoading;
class vtkMaterialInterfaceCommBuffer;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkMaterialInterfaceFilter
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMaterialInterfaceFilter* New();
  vtkTypeMacro(vtkMaterialInterfaceFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // PARAVIEW interface stuff

  /// Material sellection
  /**
   * Add a single array
   */
  void SelectMaterialArray(const char* name);
  /**
   * remove a single array
   */
  void UnselectMaterialArray(const char* name);
  /**
   * remove all arrays
   */
  void UnselectAllMaterialArrays();
  /**
   * Enable/disable processing on an array
   */
  void SetMaterialArrayStatus(const char* name, int status);
  //@{
  /**
   * Get enable./disable status for a given array
   */
  int GetMaterialArrayStatus(const char* name);
  int GetMaterialArrayStatus(int index);
  //@}
  /**
   * Query the number of available arrays
   */
  int GetNumberOfMaterialArrays();
  /**
   * Get the name of a specific array
   */
  const char* GetMaterialArrayName(int index);

  /// Mass sellection
  /**
   * Add a single array
   */
  void SelectMassArray(const char* name);
  /**
   * remove a single array
   */
  void UnselectMassArray(const char* name);
  /**
   * remove all arrays
   */
  void UnselectAllMassArrays();
  /**
   * Enable/disable processing on an array
   */
  void SetMassArrayStatus(const char* name, int status);
  //@{
  /**
   * Get enable./disable status for a given array
   */
  int GetMassArrayStatus(const char* name);
  int GetMassArrayStatus(int index);
  //@}
  /**
   * Query the number of available arrays
   */
  int GetNumberOfMassArrays();
  /**
   * Get the name of a specific array
   */
  const char* GetMassArrayName(int index);

  /// Volume weighted average attribute sellection
  /**
   * Add a single array
   */
  void SelectVolumeWtdAvgArray(const char* name);
  /**
   * remove a single array
   */
  void UnselectVolumeWtdAvgArray(const char* name);
  /**
   * remove all arrays
   */
  void UnselectAllVolumeWtdAvgArrays();

  /**
   * Enable/disable processing on an array
   */
  void SetVolumeWtdAvgArrayStatus(const char* name, int status);
  //@{
  /**
   * Get enable./disable status for a given array
   */
  int GetVolumeWtdAvgArrayStatus(const char* name);
  int GetVolumeWtdAvgArrayStatus(int index);
  //@}
  /**
   * Query the number of available arrays
   */
  int GetNumberOfVolumeWtdAvgArrays();
  /**
   * Get the name of a specific array
   */
  const char* GetVolumeWtdAvgArrayName(int index);

  /// Mass weighted average attribute sellection
  /**
   * Add a single array
   */
  void SelectMassWtdAvgArray(const char* name);
  /**
   * remove a single array
   */
  void UnselectMassWtdAvgArray(const char* name);
  /**
   * remove all arrays
   */
  void UnselectAllMassWtdAvgArrays();

  /**
   * Enable/disable processing on an array
   */
  void SetMassWtdAvgArrayStatus(const char* name, int status);
  //@{
  /**
   * Get enable./disable status for a given array
   */
  int GetMassWtdAvgArrayStatus(const char* name);
  int GetMassWtdAvgArrayStatus(int index);
  //@}
  /**
   * Query the number of available arrays
   */
  int GetNumberOfMassWtdAvgArrays();
  /**
   * Get the name of a specific array
   */
  const char* GetMassWtdAvgArrayName(int index);

  /// Summation attribute sellection
  /**
   * Add a single array
   */
  void SelectSummationArray(const char* name);
  /**
   * remove a single array
   */
  void UnselectSummationArray(const char* name);
  /**
   * remove all arrays
   */
  void UnselectAllSummationArrays();
  /**
   * Enable/disable processing on an array
   */
  void SetSummationArrayStatus(const char* name, int status);
  //@{
  /**
   * Get enable./disable status for a given array
   */
  int GetSummationArrayStatus(const char* name);
  int GetSummationArrayStatus(int index);
  //@}
  /**
   * Query the number of available arrays
   */
  int GetNumberOfSummationArrays();
  /**
   * Get the name of a specific array
   */
  const char* GetSummationArrayName(int index);

  /// Volume Fraction
  //@{
  /**
   * Volume fraction which volxels are included in a frgament.
   */
  void SetMaterialFractionThreshold(double fraction);
  vtkGetMacro(MaterialFractionThreshold, double);
  //@}

  /// OBB
  //@{
  /**
   * Turn on/off OBB calculations
   */
  vtkSetMacro(ComputeOBB, bool);
  vtkGetMacro(ComputeOBB, bool);
  //@}

  /// Loading
  //@{
  /**
   * Set the upper bound(in number of polygons) that will
   * be used to exclude processes from work sharing
   * during memory intensive portions of the algorithm.
   * acceptable values are [1 INF), however the default
   * is 1,000,000 polys. Increasing increases parallelism
   * while decreasing reduces parallelism. Setting too low
   * can cause problems. For instance if it's set so low
   * that all processes are excluded.
   */
  void SetUpperLoadingBound(int nPolys);
  vtkGetMacro(UpperLoadingBound, int);
  //@}

  /// Output file
  //@{
  /**
   * Name the file to save a table of fragment attributes to.
   */
  vtkSetStringMacro(OutputBaseName);
  vtkGetStringMacro(OutputBaseName);
  //@}
  //@{
  /**
   * If true, save the results of the filter in a text file
   */
  vtkSetMacro(WriteGeometryOutput, bool);
  vtkGetMacro(WriteGeometryOutput, bool);
  vtkSetMacro(WriteStatisticsOutput, bool);
  vtkGetMacro(WriteStatisticsOutput, bool);
  //@}

  //@{
  /**
   * Variable used to specify the number of ghost level
   * is available in each block.
   * By Default set to 1 which is what the scth reader provides
   */
  vtkSetMacro(BlockGhostLevel, unsigned char);
  vtkGetMacro(BlockGhostLevel, unsigned char);
  //@}

  /**
   * Sets modified if array selection changes.
   */
  static void SelectionModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*);

  //@{
  /**
   * Set the clip function which can be a plane or a sphere
   */
  void SetClipFunction(vtkImplicitFunction* clipFunction);
  vtkGetObjectMacro(ClipFunction, vtkImplicitFunction);
  //@}

  //@{
  /**
   * Invert the volume fraction to extract the negative space.
   * This is useful for extracting a crater.
   */
  vtkSetMacro(InvertVolumeFraction, int);
  vtkGetMacro(InvertVolumeFraction, int);
  //@}

  /**
   * Return the mtime also considering the locator and clip function.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkMaterialInterfaceFilter();
  ~vtkMaterialInterfaceFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  // Set up the result arrays for the calculations we are about to
  // make.
  void PrepareForPass(vtkNonOverlappingAMR* hbdsInput,
    std::vector<std::string>& volumeWtdAvgArrayNames,
    std::vector<std::string>& massWtdAvgArrayNames, std::vector<std::string>& summedArrayNames,
    std::vector<std::string>& integratedArrayNames);
  // Create a new fragment/piece.
  vtkPolyData* NewFragmentMesh();
  // Process each cell, looking for fragments.
  int ProcessBlock(int blockId);
  // Cell has been identified as inside the fragment. Integrate, and
  // generate fragment surface etc...
  void ConnectFragment(vtkMaterialInterfaceFilterRingBuffer* iterator);
  void GetNeighborIterator(vtkMaterialInterfaceFilterIterator* next,
    vtkMaterialInterfaceFilterIterator* iterator, int axis0, int maxFlag0, int axis1, int maxFlag1,
    int axis2, int maxFlag2);
  void GetNeighborIteratorPad(vtkMaterialInterfaceFilterIterator* next,
    vtkMaterialInterfaceFilterIterator* iterator, int axis0, int maxFlag0, int axis1, int maxFlag1,
    int axis2, int maxFlag2);
  void CreateFace(vtkMaterialInterfaceFilterIterator* in, vtkMaterialInterfaceFilterIterator* out,
    int axis, int outMaxFlag);
  int ComputeDisplacementFactors(vtkMaterialInterfaceFilterIterator* pointNeighborIterators[8],
    double displacmentFactors[3], int rootNeighborIdx, int faceAxis);
  int SubVoxelPositionCorner(double* point,
    vtkMaterialInterfaceFilterIterator* pointNeighborIterators[8], int rootNeighborIdx,
    int faceAxis);
  void FindPointNeighbors(vtkMaterialInterfaceFilterIterator* iteratorMin0,
    vtkMaterialInterfaceFilterIterator* iteratorMax0, int axis0, int maxFlag1, int maxFlag2,
    vtkMaterialInterfaceFilterIterator pointNeighborIterators[8], double pt[3]);
  // Finds a global origin for the data set, and level 0 dx
  int ComputeOriginAndRootSpacingOld(vtkNonOverlappingAMR* input);
  void ComputeOriginAndRootSpacing(vtkNonOverlappingAMR* input);
  // Returns the total number of local(wrt this proc) blocks.
  int GetNumberOfLocalBlocks(vtkNonOverlappingAMR* input);
  // Complex ghost layer Handling.
  std::vector<vtkMaterialInterfaceFilterBlock*> GhostBlocks;
  void ShareGhostBlocks();
  void HandleGhostBlockRequests();
  int ComputeRequiredGhostExtent(int level, int inExt[6], int outExt[6]);

  void ComputeAndDistributeGhostBlocks(
    int* numBlocksInProc, int* blockMetaData, int myProc, int numProcs);

  vtkMultiProcessController* Controller;

  vtkMaterialInterfaceEquivalenceSet* EquivalenceSet;
  void AddEquivalence(
    vtkMaterialInterfaceFilterIterator* neighbor1, vtkMaterialInterfaceFilterIterator* neighbor2);
  //
  void PrepareForResolveEquivalences();
  //
  void ResolveEquivalences();
  void GatherEquivalenceSets(vtkMaterialInterfaceEquivalenceSet* set);
  void ShareGhostEquivalences(vtkMaterialInterfaceEquivalenceSet* globalSet, int* procOffsets);
  void ReceiveGhostFragmentIds(vtkMaterialInterfaceEquivalenceSet* globalSet, int* procOffset);
  void MergeGhostEquivalenceSets(vtkMaterialInterfaceEquivalenceSet* globalSet);

  // Sum/finalize attribute's contribution for those
  // which are split over multiple processes.
  int ResolveIntegratedAttributes(const int controllingProcId);
  // Initialize our attribute arrays to ho9ld resolved attributes
  int PrepareToResolveIntegratedAttributes();

  // Send my integrated attributes to another process.
  int SendIntegratedAttributes(const int recipientProcId);
  // Receive integrated attributes from another process.
  int ReceiveIntegratedAttributes(const int sourceProcId);
  // Size buffers etc...
  int PrepareToCollectIntegratedAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& volumes, std::vector<vtkDoubleArray*>& clipDepthMaxs,
    std::vector<vtkDoubleArray*>& clipDepthMins, std::vector<vtkDoubleArray*>& moments,
    std::vector<std::vector<vtkDoubleArray*> >& volumeWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& massWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& sums);
  // Free resources.
  int CleanUpAfterCollectIntegratedAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& volumes, std::vector<vtkDoubleArray*>& clipDepthMaxs,
    std::vector<vtkDoubleArray*>& clipDepthMins, std::vector<vtkDoubleArray*>& moments,
    std::vector<std::vector<vtkDoubleArray*> >& volumeWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& massWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& sums);
  // Receive all integrated attribute arrays from all other
  // processes.
  int CollectIntegratedAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& volumes, std::vector<vtkDoubleArray*>& clipDepthMaxs,
    std::vector<vtkDoubleArray*>& clipDepthMins, std::vector<vtkDoubleArray*>& moments,
    std::vector<std::vector<vtkDoubleArray*> >& volumeWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& massWtdAvgs,
    std::vector<std::vector<vtkDoubleArray*> >& sums);
  // Send my integrated attributes to all other processes.
  int BroadcastIntegratedAttributes(const int sourceProcessId);
  // Send my geometric attribuites to a controller.
  int SendGeometricAttributes(const int controllingProcId);
  // size buffers & new containers
  int PrepareToCollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& coaabb, std::vector<vtkDoubleArray*>& obb,
    std::vector<int*>& ids);
  // Free resources.
  int CleanUpAfterCollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& coaabb, std::vector<vtkDoubleArray*>& obb,
    std::vector<int*>& ids);
  // Receive all geometric attributes from all other
  // processes.
  int CollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<vtkDoubleArray*>& coaabb, std::vector<vtkDoubleArray*>& obb,
    std::vector<int*>& ids);
  // size local copy to hold all.
  int PrepareToMergeGeometricAttributes();
  // Gather geometric attributes on a single process.
  int GatherGeometricAttributes(const int recipientProcId);
  // Merge fragment's geometry that are split on this process
  void ResolveLocalFragmentGeometry();
  // Merge fragment's geometry that are split across processes
  void ResolveRemoteFragmentGeometry();
  // Clean duplicate points from fragment geometry.
  void CleanLocalFragmentGeometry();
  //
  void BuildLoadingArray(std::vector<vtkIdType>& loadingArray);
  int PackLoadingArray(vtkIdType*& buffer);
  int UnPackLoadingArray(vtkIdType* buffer, int bufSize, std::vector<vtkIdType>& loadingArray);

  // copy any integrated attributes (volume, id, weighted averages, sums, etc)
  // into the fragment polys in the output data sets.
  void CopyAttributesToOutput0();
  void CopyAttributesToOutput1();
  void CopyAttributesToOutput2();
  // Write a text file containing local fragment attributes.
  int WriteGeometryOutputToTextFile();
  int WriteStatisticsOutputToTextFile();
  // Build the output data
  int BuildOutputs(vtkMultiBlockDataSet* mbdsOutput0, vtkMultiBlockDataSet* mbdsOutput1,
    vtkMultiBlockDataSet* mbdsOutput2, int materialId);

  // integration helper, returns 0 if the source array
  // type is unsupported.
  int Accumulate(double* dest,           // scalar/vector result
    vtkDataArray* src,                   // array to accumulate from
    int nComps,                          //
    int srcCellIndex,                    // which cell
    double weight);                      // weight of contribution
  int AccumulateMoments(double* moments, // =(Myz, Mxz, Mxy, m)
    vtkDataArray* massArray,             //
    int srcCellIndex,                    // from which cell in mass
    double* X);
  // Compute the geomteric attributes that have been requested.
  void ComputeGeometricAttributes();
  int ComputeLocalFragmentOBB();
  int ComputeLocalFragmentAABBCenters();
  // int ComputeFragmentMVBB();

  // Format input block into an easy to access array with
  // extra metadata (information) extracted.
  int NumberOfInputBlocks;
  vtkMaterialInterfaceFilterBlock** InputBlocks;
  void DeleteAllBlocks();
  int InitializeBlocks(vtkNonOverlappingAMR* input, std::string& materialFractionArrayName,
    std::string& massArrayName, std::vector<std::string>& volumeWtdAvgArrayNames,
    std::vector<std::string>& massWtdAvgArrayNames, std::vector<std::string>& summedArrayNames,
    std::vector<std::string>& integratedArrayNames);
  void AddBlock(vtkMaterialInterfaceFilterBlock* block, unsigned char levelOfGhostLayer);

  // New methods for connecting neighbors.
  void CheckLevelsForNeighbors(vtkMaterialInterfaceFilterBlock* block);
  // Returns 1 if there we neighbors found, 0 if not.
  int FindFaceNeighbors(unsigned int blockLevel, int blockIndex[3], int faceAxis, int faceMaxFlag,
    std::vector<vtkMaterialInterfaceFilterBlock*>* result);

  // We need ghost cells for edges and corners as well as faces.
  // neighborDirection is used to specify a face, edge or corner.
  // Using a 2x2x2 cube center at origin: (-1,-1,-1), (-1,-1,1) ... are corners.
  // (1,1,0) is an edge, and (-1,0,0) is a face.
  // Returns 1 if the neighbor exists.
  int HasNeighbor(unsigned int blockLevel, int blockIndex[3], int neighborDirection[3]);

  // Threshold value used to select a cell
  // as being iniside some fragment, PV uses
  // a double between 0 and 1, this is stored here
  double MaterialFractionThreshold;
  // The extraction filter uses a scaled threshold
  // in the range of 0 to 255
  double scaledMaterialFractionThreshold;
  //
  char* MaterialFractionArrayName;
  vtkSetStringMacro(MaterialFractionArrayName);

  // while processing a material array this holds
  // a pointer to the output poly data
  // data set
  vtkPolyData* CurrentFragmentMesh;
  // As pieces/fragments are found they are stored here
  // until resolution.
  std::vector<vtkPolyData*> FragmentMeshes;

  // TODO? this could be cleaned up (somewhat) by
  // adding an integration class which encapsulates
  // all of the supported operations.
  /// class vtkMaterialInterfaceFilterIntegrator
  ///{
  // Local id of current fragment
  int FragmentId;
  // Accumulator for the volume of the current fragment.
  double FragmentVolume;
  // Fragment volumes indexed by the fragment id. It's a local
  // per-process indexing until fragments have been resolved
  vtkDoubleArray* FragmentVolumes;

  // Min and max depth of crater.
  // These are only computed when the clip plane is on.
  double ClipDepthMin;
  double ClipDepthMax;
  vtkDoubleArray* ClipDepthMinimums;
  vtkDoubleArray* ClipDepthMaximums;

  // Accumulator for moments of the current fragment
  std::vector<double> FragmentMoment; // =(Myz, Mxz, Mxy, m)
  // Moments indexed by fragment id
  vtkDoubleArray* FragmentMoments;
  // Centers of fragment AABBs, only computed if moments are not
  vtkDoubleArray* FragmentAABBCenters;
  // let us know if the user has specified a mass array
  bool ComputeMoments;

  // Weighted average, where weights correspond to fragment volume.
  // Accumulators one for each array to average, scalar or vector
  std::vector<std::vector<double> > FragmentVolumeWtdAvg;
  // weighted averages indexed by fragment id.
  std::vector<vtkDoubleArray*> FragmentVolumeWtdAvgs;
  // number of arrays for which to compute the weighted average
  int NVolumeWtdAvgs;
  // Names of the arrays to average.
  std::vector<std::string> VolumeWtdAvgArrayNames;

  // Weighted average, where weights correspond to fragment mass.
  // Accumulators one for each array to average, scalar or vector
  std::vector<std::vector<double> > FragmentMassWtdAvg;
  // weighted averages indexed by fragment id.
  std::vector<vtkDoubleArray*> FragmentMassWtdAvgs;
  // number of arrays for which to compute the weighted average
  int NMassWtdAvgs;
  // Names of the arrays to average.
  std::vector<std::string> MassWtdAvgArrayNames;

  // Unique list of all integrated array names
  // it's used construct list of arrays that
  // will be copied into output.
  std::vector<std::string> IntegratedArrayNames;
  std::vector<int> IntegratedArrayNComp;
  // number of integrated arrays
  int NToIntegrate;

  // Sum of data over the fragment.
  // Accumulators, one for each array to sum
  std::vector<std::vector<double> > FragmentSum;
  // sums indexed by fragment id.
  std::vector<vtkDoubleArray*> FragmentSums;
  // number of arrays for which to compute the weighted average
  int NToSum;
  ///};

  // OBB indexed by fragment id
  vtkDoubleArray* FragmentOBBs;
  // turn on/off OBB calculation
  bool ComputeOBB;

  // Upper bound used to exclude heavily loaded procs
  // from work sharing. Reducing may aliviate oom issues.
  int UpperLoadingBound;

  // This is getting a bit ugly but ...
  // When we resolve (merge equivalent) fragments we need a mapping
  // from local ids to global ids.
  // This array give an offset into the global array for each process.
  // The array is computed when we resolve ids, and is used
  // when resoving other attributes like volume
  int* NumberOfRawFragmentsInProcess; // in each process by proc id for a single material
  int* LocalToGlobalOffsets;          // indexes into a gathered array of local ids by proc id
  int TotalNumberOfRawFragments;      // over all processes for a single material
  int NumberOfResolvedFragments;      // over all processes for a single material
  // Total number of fragments over all materials, used to
  // generate a unique id for each fragment.
  int ResolvedFragmentCount;
  // Material id, each pass involves a different material use this to
  // tag fragments by material.
  int MaterialId;

  // For each material an array of resolved fragments. Blocks are multi piece
  // of poly data. The multipiece is much like a std vector of poly data *.
  // multi block is indexed by material.
  vtkMultiBlockDataSet* ResolvedFragments;
  // for each material a list of global ids of pieces we own.
  std::vector<std::vector<int> > ResolvedFragmentIds;
  // List of split fragments
  std::vector<std::vector<int> > FragmentSplitMarker;
  vtkIntArray* FragmentSplitGeometry;

  // A polydata with points at fragment centers, same structure
  // as the resolved fragments.
  vtkMultiBlockDataSet* ResolvedFragmentCenters;
  //
  std::vector<vtkPoints*> ResolvedFragmentPoints;

  // A polydata representing OBB, same structure as the resolved
  // fragments.
  vtkMultiBlockDataSet* ResolvedFragmentOBBs;

  double GlobalOrigin[3];
  double RootSpacing[3];
  int StandardBlockDimensions[3];

  void SaveBlockSurfaces(const char* fileName);
  void SaveGhostSurfaces(const char* fileName);

  // Use for the moment to find neighbors.
  // It could be changed into the primary storage of blocks.
  std::vector<vtkMaterialInterfaceLevel*> Levels;

  // Ivars for computing the point on corners and edges of a face.
  vtkMaterialInterfaceFilterIterator* FaceNeighbors;
  // Permutation of the neighbors. Axis0 normal to face.
  int faceAxis0;
  int faceAxis1;
  int faceAxis2;
  double FaceCornerPoints[12];
  double FaceEdgePoints[12];
  int FaceEdgeFlags[4];
  // outMaxFlag implies out is positive direction of axis.
  void ComputeFacePoints(vtkMaterialInterfaceFilterIterator* in,
    vtkMaterialInterfaceFilterIterator* out, int axis, int outMaxFlag);
  void ComputeFaceNeighbors(vtkMaterialInterfaceFilterIterator* in,
    vtkMaterialInterfaceFilterIterator* out, int axis, int outMaxFlag);

  long ComputeProximity(const int faceIdx[3], int faceLevel, const int ext[6], int refLevel);

  void FindNeighbor(int faceIndex[3], int faceLevel, vtkMaterialInterfaceFilterIterator* neighbor,
    vtkMaterialInterfaceFilterIterator* reference);

  // PARAVIEW interface data
  vtkDataArraySelection* MaterialArraySelection;
  vtkDataArraySelection* MassArraySelection;
  vtkDataArraySelection* VolumeWtdAvgArraySelection;
  vtkDataArraySelection* MassWtdAvgArraySelection;
  vtkDataArraySelection* SummationArraySelection;
  vtkCallbackCommand* SelectionObserver;
  char* OutputBaseName;
  bool WriteGeometryOutput;
  bool WriteStatisticsOutput;
  int DrawOBB;
  double Progress;
  double ProgressMaterialInc;
  double ProgressBlockInc;
  double ProgressResolutionInc;
// Debug
#ifdef vtkMaterialInterfaceFilterDEBUG
  int MyPid;
#endif

  vtkImplicitFunction* ClipFunction;

  // Variables that setup clipping with a sphere and a plane.
  double ClipCenter[3];
  int ClipWithSphere;
  double ClipRadius;
  int ClipWithPlane;
  double ClipPlaneVector[3];
  double ClipPlaneNormal[3];

  // Variable that will invert a material.
  int InvertVolumeFraction;

  // Specify the number of Ghost level available in each block.
  // By default set to 1
  unsigned char BlockGhostLevel;

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  vtkSmartPointer<vtkTimerLog> InitializeBlocksTimer;
  vtkSmartPointer<vtkTimerLog> ShareGhostBlocksTimer;
  long NumberOfBlocks;
  long NumberOfGhostBlocks;
  vtkSmartPointer<vtkTimerLog> ProcessBlocksTimer;
  vtkSmartPointer<vtkTimerLog> ResolveEquivalencesTimer;
#endif

private:
  vtkMaterialInterfaceFilter(const vtkMaterialInterfaceFilter&) = delete;
  void operator=(const vtkMaterialInterfaceFilter&) = delete;
};

#endif
