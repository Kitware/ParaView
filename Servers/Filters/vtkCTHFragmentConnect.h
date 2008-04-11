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

#include "vtkPolyDataAlgorithm.h"
#include "vtkstd/vector" // using vector internally. ok for leaf classes.

class vtkDoubleArray;
class vtkCellArray;
class vtkImageData;
class vtkPoints;
class vtkHierarchicalBoxDataSet;
class vtkCTHFragmentLevel;
class vtkCTHFragmentConnectBlock;
class vtkCTHFragmentConnectIterator;
class vtkCTHFragmentEquivalenceSet;
class vtkCTHFragmentConnectRingBuffer;
class vtkMultiProcessController;

class VTK_EXPORT vtkCTHFragmentConnect : public vtkPolyDataAlgorithm
{
public:
  static vtkCTHFragmentConnect *New();
  vtkTypeRevisionMacro(vtkCTHFragmentConnect,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The filter only processes one array at a time for now.
  vtkSetStringMacro(VolumeFractionArrayName);
  vtkGetStringMacro(VolumeFractionArrayName);

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
  // Returns the total number of blocks in all levels (local process only).
  int  ComputeOriginAndRootSpacing(
        vtkHierarchicalBoxDataSet* input);

  char *VolumeFractionArrayName;

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
  
  vtkPolyData* Mesh;
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
  void ResolveVolumes();
  void GenerateVolumeArray(
    vtkIntArray* fragemntIds,
    vtkPolyData *output);
  
  // Format input block into an easy to access array with
  // extra metadata (information) extracted.
  int NumberOfInputBlocks;
  vtkCTHFragmentConnectBlock** InputBlocks;
  void DeleteAllBlocks();
  int InitializeBlocks(vtkImageData* input); 
  int InitializeBlocks(vtkHierarchicalBoxDataSet* input); 
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

  vtkIntArray *BlockIdArray;
  vtkIntArray *LevelArray;
  
  int FragmentId;
  // Integrate the volume for this fragment.
  // We will do the same for all attributes?
  double FragmentVolume;
  // Save the volume in this array indexed by the fragmentId.
  vtkDoubleArray* FragmentVolumes;

  // I am going to try to integrate the cell attriubtes here.
  // The simplest thing to do is keep the arrays in a vtkCellData object.
  vtkCellData* IntegratedFragmentAttributes;
  // I am going to do the actual integration in a raw memory buffer.
  // It is flexible with no complicated arbitrary structure.
  // I just iterate through the buffer casting the pointer to the correct types.
  void* IntegrationBuffer;
  
  
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

private:
  vtkCTHFragmentConnect(const vtkCTHFragmentConnect&);  // Not implemented.
  void operator=(const vtkCTHFragmentConnect&);  // Not implemented.
  //ETX
};

#endif
