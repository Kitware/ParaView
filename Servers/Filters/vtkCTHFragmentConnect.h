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
#include "vtkstd/vector"

class vtkCellArray;
class vtkImageData;
class vtkPoints;
class vtkHierarchicalDataSet;
class vtkCTHFragmentLevel;
class vtkCTHFragmentConnectBlock;
class vtkCTHFragmentConnectIterator;
class vtkCTHFragmentEquivalenceSet;
class vtkMultiProcessController;

class VTK_GRAPHICS_EXPORT vtkCTHFragmentConnect : public vtkPolyDataAlgorithm
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

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int ProcessBlock(int blockId);

  void ConnectFragment(vtkCTHFragmentConnectIterator* iterator);
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
        vtkCTHFragmentConnectIterator* iterator,
        int axis, int maxFlag,
        vtkCTHFragmentConnectIterator* next);
  void ComputeDisplacementFactors(
        vtkCTHFragmentConnectIterator* pointNeighborIterators, 
        double displacmentFactors[3]);
  void ComputeCorner(
        double* point, 
        vtkCTHFragmentConnectIterator pointNeighborIterators[8]);
  void FindPointNeighbors(
        vtkCTHFragmentConnectIterator* iteratorMin0, 
        vtkCTHFragmentConnectIterator* iteratorMax0,
        int axis0, int maxFlag1, int maxFlag2, 
        vtkCTHFragmentConnectIterator pointNeighborIterators[8]);
  // Returns the total number of blocks in all levels (local process only).
  int  ComputeOriginAndRootSpacing(
        vtkHierarchicalDataSet* input);

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
  
  // These ivars ar to save stack space for recursive functions.
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
    int *offsets);




  // Format input block into an easy to access array with
  // extra metadata (information) extracted.
  int NumberOfInputBlocks;
  vtkCTHFragmentConnectBlock** InputBlocks;
  void DeleteAllBlocks();
  int InitializeBlocks(vtkImageData* input); 
  int InitializeBlocks(vtkHierarchicalDataSet* input); 
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

  double GlobalOrigin[3];
  double RootSpacing[3];
  int StandardBlockDimensions[3];

  // Limit recursion depth to avoid stack overflow.
  int RecursionDepth;
  int MaximumRecursionDepth;

  void SaveBlockSurfaces(const char* fileName);
  void SaveGhostSurfaces(const char* fileName);

  // Use for the moment to find neighbors.
  // It could be changed into the primary storage of blocks.
  vtkstd::vector<vtkCTHFragmentLevel*> Levels;

private:
  vtkCTHFragmentConnect(const vtkCTHFragmentConnect&);  // Not implemented.
  void operator=(const vtkCTHFragmentConnect&);  // Not implemented.
};

#endif
