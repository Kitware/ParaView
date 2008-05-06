/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentConnect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentConnect - Extract particles and analyse them.
// .SECTION Description
// This filter takes a cell data volume fraction and generates a polydata
// surface.  It also performs connectivity on the particles and generates
// a particle index as part of the cell data of the output.  It computes
// the volume of each particle from the volume fraction.

#ifndef __vtkCTHFragmentConnect_h
#define __vtkCTHFragmentConnect_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkstd/vector" // using vector internally. ok for leaf classes.
#include "vtkstd/string" // ...then same is true of string

class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkHierarchicalBoxDataSet;
class vtkPoints;
class vtkDoubleArray;
class vtkCellArray;
class vtkCellData;
class vtkIntArray;
class vtkCTHFragmentLevel;
class vtkCTHFragmentConnectBlock;
class vtkCTHFragmentConnectIterator;
class vtkCTHFragmentEquivalenceSet;
class vtkCTHFragmentConnectRingBuffer;
class vtkCTHMaterialFragmentArray;
class vtkMultiProcessController;
class vtkDataArraySelection;
class vtkCallbackCommand;


class VTK_EXPORT vtkCTHFragmentConnect : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCTHFragmentConnect *New();
  vtkTypeRevisionMacro(vtkCTHFragmentConnect,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // PARAVIEW interface stuff

  // Material sellection
  // Add a single array
  void SelectMaterialArray(const char *name);
  // remove a single array
  void UnselectMaterialArray( const char *name );
  // remove all arrays
  void UnselectAllMaterialArrays();

  // Enable/disable processing on an array
  void SetMaterialArrayStatus( const char* name,
                               int status );
  // Get enable./disable status for a given array
  int GetMaterialArrayStatus(const char* name);
  int GetMaterialArrayStatus(int index);
  // Query the number of available arrays
  int GetNumberOfMaterialArrays();
  // Get the name of a specific array
  const char *GetMaterialArrayName(int index);

  // WeightedAverage attribute sellection
  // Add a single array
  void SelectWeightedAverageArray(const char *name);
  // remove a single array
  void UnselectWeightedAverageArray( const char *name );
  // remove all arrays
  void UnselectAllWeightedAverageArrays();

  // Enable/disable processing on an array
  void SetWeightedAverageArrayStatus( const char* name,
                                  int status );
  // Get enable./disable status for a given array
  int GetWeightedAverageArrayStatus(const char* name);
  int GetWeightedAverageArrayStatus(int index);
  // Query the number of available arrays
  int GetNumberOfWeightedAverageArrays();
  // Get the name of a specific array
  const char *GetWeightedAverageArrayName(int index);

  // Summation attribute sellection
  // Add a single array
  void SelectSummationArray(const char *name);
  // remove a single array
  void UnselectSummationArray( const char *name );
  // remove all arrays
  void UnselectAllSummationArrays();

  // Enable/disable processing on an array
  void SetSummationArrayStatus( const char* name,
                                  int status );
  // Get enable./disable status for a given array
  int GetSummationArrayStatus(const char* name);
  int GetSummationArrayStatus(int index);
  // Query the number of available arrays
  int GetNumberOfSummationArrays();
  // Get the name of a specific array
  const char *GetSummationArrayName(int index);



  // volume fraction 
  void SetMaterialFractionThreshold(double fraction);
  vtkGetMacro(MaterialFractionThreshold, double);

  // sets modified if array selection changes 
  static void SelectionModifiedCallback( vtkObject*,
                                         unsigned long,
                                         void* clientdata,
                                         void* );

protected:
  vtkCTHFragmentConnect();
  ~vtkCTHFragmentConnect();

  //BTX
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int ProcessBlock(int blockId);

  void ConnectFragment(vtkCTHFragmentConnectRingBuffer* iterator);
  void GetNeighborIterator(
        vtkCTHFragmentConnectIterator* next,
        vtkCTHFragmentConnectIterator* iterator, 
        int axis0, int maxFlag0,
        int axis1, int maxFlag1,
        int axis2, int maxFlag2);
  void GetNeighborIteratorPad(
        vtkCTHFragmentConnectIterator* next,
        vtkCTHFragmentConnectIterator* iterator, 
        int axis0, int maxFlag0,
        int axis1, int maxFlag1,
        int axis2, int maxFlag2);
  void CreateFace(
        vtkCTHFragmentConnectIterator* in,
        vtkCTHFragmentConnectIterator* out,
        int axis, int outMaxFlag);
  void ComputeDisplacementFactors(
        vtkCTHFragmentConnectIterator* pointNeighborIterators[8],
        double displacmentFactors[3]);
  void SubVoxelPositionCorner(
        double* point, 
        vtkCTHFragmentConnectIterator* pointNeighborIterators[8]);
  void FindPointNeighbors(
        vtkCTHFragmentConnectIterator* iteratorMin0, 
        vtkCTHFragmentConnectIterator* iteratorMax0,
        int axis0, int maxFlag1, int maxFlag2, 
        vtkCTHFragmentConnectIterator pointNeighborIterators[8],
        double pt[3]);
  // Finds a global origin for the data set, and level 0 dx
  int  ComputeOriginAndRootSpacingOld(
        vtkHierarchicalBoxDataSet* input);
  void  ComputeOriginAndRootSpacing(
        vtkHierarchicalBoxDataSet* input);
  // Returns the total number of local(wrt this proc) blocks.
  int GetNumberOfLocalBlocks(
        vtkHierarchicalBoxDataSet* input);
  // Complex ghost layer Handling.
  vtkstd::vector<vtkCTHFragmentConnectBlock*> GhostBlocks;
  void ShareGhostBlocks();
  void HandleGhostBlockRequests();
  int ComputeRequiredGhostExtent(
    int level,
    int inExt[6],
    int outExt[6]);

  void ComputeAndDistributeGhostBlocks(
    int *numBlocksInProc,
    int* blockMetaData,
    int myProc,
    int numProcs);

  vtkMultiProcessController* Controller;

  vtkCTHFragmentEquivalenceSet* EquivalenceSet;
  void AddEquivalence(
    vtkCTHFragmentConnectIterator *neighbor1,
    vtkCTHFragmentConnectIterator *neighbor2);
  void ResolveEquivalences(vtkIntArray* fragmentIdArray);
  void GatherEquivalenceSets(vtkCTHFragmentEquivalenceSet* set);
  void ShareGhostEquivalences(
    vtkCTHFragmentEquivalenceSet* globalSet,
    int*  procOffsets);
  void ReceiveGhostFragmentIds(
    vtkCTHFragmentEquivalenceSet* globalSet,
    int* procOffset);
  void MergeGhostEquivalenceSets(
    vtkCTHFragmentEquivalenceSet* globalSet);
  void ResolveAndPartitionFragments();
  // copy any integrated attribute(including volume)
  // into the fragment polys in the output data set
  void CopyIntegratedAttributesToFragments(
    vtkIntArray* fragmentIds,
    vtkDataSet* output );
  // integration helper, returns 0 if the source array 
  // type is unsupported.
  int Accumulate(vtkstd::vector<double> &dest, // scalar/vector result
                 vtkDataArray *src,    // array to accumulate from
                 int nComps,           // 
                 int srcCellIndex,     // which cell
                 double weight);       // weight of contribution


  // Format input block into an easy to access array with
  // extra metadata (information) extracted.
  int NumberOfInputBlocks;
  vtkCTHFragmentConnectBlock** InputBlocks;
  void DeleteAllBlocks();
  int InitializeBlocks( vtkHierarchicalBoxDataSet* input, 
                        vtkstd::string &materialFractionArrayName,
                        vtkstd::vector<vtkstd::string> &averagedArrayNames,
                        vtkstd::vector<vtkstd::string> &summedArrayNames );
  void AddBlock(vtkCTHFragmentConnectBlock* block);

  // New methods for connecting neighbors.
  void CheckLevelsForNeighbors(
    vtkCTHFragmentConnectBlock* block);
  // Returns 1 if there we neighbors found, 0 if not.
  int FindFaceNeighbors(
    unsigned int blockLevel,
    int blockIndex[3],
    int faceAxis,
    int faceMaxFlag,
    vtkstd::vector<vtkCTHFragmentConnectBlock*> *result);

  // We need ghost cells for edges and corners as well as faces.
  // neighborDirection is used to specify a face, edge or corner.
  // Using a 2x2x2 cube center at origin: (-1,-1,-1), (-1,-1,1) ... are corners.
  // (1,1,0) is an edge, and (-1,0,0) is a face.
  // Returns 1 if the neighbor exists.
  int HasNeighbor(
    unsigned int blockLevel,
    int blockIndex[3],
    int neighborDirection[3]);

  // Threshold value used to select a cell
  // as being iniside some fragment, PV uses
  // a double between 0 and 1, this is stored here
  double MaterialFractionThreshold;
  // The extraction filter uses a scaled threshold
  // in the range of 0 to 255
  double scaledMaterialFractionThreshold;

  vtkIntArray *BlockIdArray;
  vtkIntArray *LevelArray;
  // while processing a material array this holds
  // a pointer to the output poly data
  // data set, At other time its not valid
  vtkPolyData *CurrentFragmentMesh;
  // while processing an individual material 
  // this points to the name of the fragment array
  // At other times its not valid
  const char *CurrentFragmentIdArrayName;

  // Local id of current fragment
  int FragmentId;

  // Accumulator for the volume of the current fragment.
  double FragmentVolume;
  // Fragment volumes indexed by the fragment id. It's a local
  // per-process indexing until fragments have been resolved
  vtkDoubleArray* FragmentVolumes;

  // Weighted average, where weights correspond to fragment volume.
  // Accumulators one for each array to average
  vtkstd::vector<vtkstd::vector<double> > FragmentWeightedAverage;
  // Final weighted averages are indexed by fragment id.
  vtkstd::vector<vtkDoubleArray *>FragmentWeightedAverages;
  // number of arrays for which to compute the weighted average
  int nToAverage;

  // Sum of data over the fragment.
  // Accumulators, one for each array to sum
  vtkstd::vector<vtkstd::vector<double> > FragmentSum;
  // Final weighted averages are indexed by fragment id.
  vtkstd::vector<vtkDoubleArray *>FragmentSums;
  // number of arrays for which to compute the weighted average
  int nToSum;


//   // I am going to try to integrate the cell attriubtes here.
//   // The simplest thing to do is keep the arrays in a vtkCellData object.
//   vtkCellData* IntegratedFragmentAttributes;
//   // I am going to do the actual integration in a raw memory buffer.
//   // It is flexible with no complicated arbitrary structure.
//   // I just iterate through the buffer casting the pointer to the correct types.
//   void* IntegrationBuffer;

  // This is getting a bit ugly but ...
  // When we resolve (merge equivalent) fragments we need a mapping
  // from local ids to global ids.
  // This array give an offset into the global array for each process.
  // The array is computed when we resolve ids, and is used 
  // when resoving other attributes like volume
  int *NumberOfRawFragmentsInProcess;  // in each process.
  int *LocalToGlobalOffsets;
  int TotalNumberOfRawFragments;
  int NumberOfResolvedFragments;

  double GlobalOrigin[3];
  double RootSpacing[3];
  int StandardBlockDimensions[3];

  void SaveBlockSurfaces(const char* fileName);
  void SaveGhostSurfaces(const char* fileName);

  // Use for the moment to find neighbors.
  // It could be changed into the primary storage of blocks.
  vtkstd::vector<vtkCTHFragmentLevel*> Levels;

  // Ivars for computing the point on corners and edges of a face.
  vtkCTHFragmentConnectIterator* FaceNeighbors;
  // Permutation of the neighbors. Axis0 normal to face.
  int faceAxis0;
  int faceAxis1;
  int faceAxis2;
  double FaceCornerPoints[12];
  double FaceEdgePoints[12];
  int    FaceEdgeFlags[4];
  // outMaxFlag implies out is positive direction of axis.
  void ComputeFacePoints(vtkCTHFragmentConnectIterator* in,
                        vtkCTHFragmentConnectIterator* out,
                        int axis, int outMaxFlag);
  void ComputeFaceNeighbors(vtkCTHFragmentConnectIterator* in,
                            vtkCTHFragmentConnectIterator* out,
                            int axis, int  outMaxFlag);
  void FindNeighbor(
    int faceIndex[3], int faceLevel, 
    vtkCTHFragmentConnectIterator* neighbor,
    vtkCTHFragmentConnectIterator* reference);

  // Create an array of loading factors a measure of the process work
  // load that each fragment piece represents. Indexed by fragement id.
  // These are used for load ballancing purposes as fragment pieces are 
  // collected.
  vtkIntArray *LoadArray;
  void BuildLocalLoadArray(vtkIntArray *loadArray);

  // PARAVIEW interface data
  vtkDataArraySelection *MaterialArraySelection;
  vtkDataArraySelection *WeightedAverageArraySelection;
  vtkDataArraySelection *SummationArraySelection;
  vtkCallbackCommand *SelectionObserver;

private:
  vtkCTHFragmentConnect(const vtkCTHFragmentConnect&);  // Not implemented.
  void operator=(const vtkCTHFragmentConnect&);  // Not implemented.
  //ETX
};

#endif
