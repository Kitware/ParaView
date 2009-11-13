/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDualGridHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRDualGridHelper - Tools for processing AMR as a dual grid.
// .SECTION Description
// This helper object was developed to help the AMR dual grid connectivity
// and integration filter but I also want a dual grid iso surface filter
// so I mad it a separate class.  The API needs to be improved to make
// it more generally useful.

#ifndef __vtkAMRDualGridHelper_h
#define __vtkAMRDualGridHelper_h

#include "vtkObject.h"
#include "vtkstd/vector"

class vtkHierarchicalBoxDataSet;
class vtkAMRDualGridHelperBlock;
class vtkAMRDualGridHelperLevel;
class vtkMultiProcessController;
class vtkImageData;
class vtkAMRDualGridHelperDegenerateRegion;
class vtkAMRDualGridHelperFace;

//----------------------------------------------------------------------------
class VTK_EXPORT vtkAMRDualGridHelper : public vtkObject
{
public:
  static vtkAMRDualGridHelper *New();
  vtkTypeRevisionMacro(vtkAMRDualGridHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Set this before you call initialize.
  void SetEnableDegenerateCells(int v) { this->EnableDegenerateCells = v;}

  void SetEnableMultiProcessCommunication(int v);

  int                       Initialize(vtkHierarchicalBoxDataSet* input,
                                       const char* arrayName);
  const double*             GetGlobalOrigin() { return this->GlobalOrigin;}
  const double*             GetRootSpacing() { return this->RootSpacing;}
  int                       GetNumberOfBlocks() { return this->NumberOfBlocksInThisProcess;}
  int                       GetNumberOfLevels() { return this->Levels.size();}
  int                       GetNumberOfBlocksInLevel(int level);
  vtkAMRDualGridHelperBlock* GetBlock(int level, int blockIdx);

private:
  vtkAMRDualGridHelper();
  ~vtkAMRDualGridHelper();

//BTX

  char* ArrayName;
  int DataTypeSize;
  vtkSetStringMacro(ArrayName);
  
  // Distributed execution
  void ShareMetaData();
  void ShareBlocks();
  void SendBlocks(int remoteProc, int localProc);
  void ReceiveBlocks(int remoteProc);
  void AllocateMessageBuffer(int maxLength);
  unsigned char* MessageBuffer;
  int MessageBufferLength;

  vtkMultiProcessController *Controller;
  void ComputeGlobalMetaData(vtkHierarchicalBoxDataSet* input);
  void AddBlock(int level, vtkImageData* volume);

  // Manage connectivity seeds between blocks.
  void CreateFaces();
  void FindExistingFaces(
    vtkAMRDualGridHelperBlock* block,
    int level, int x, int y, int z);

  // Assign shared cells between blocks and meshing
  // changes in level.
  void AssignSharedRegions();
  void AssignBlockSharedRegions(
    vtkAMRDualGridHelperBlock* block, int blockLevel,
    int blockX, int blockY, int blockZ);
  int ClaimBlockSharedRegion(
    vtkAMRDualGridHelperBlock* block,
    int blockX,  int blockY,  int blockZ,
    int regionX, int regionY, int regionZ);

  int NumberOfBlocksInThisProcess;

  // Cell dimension of block without ghost layers.
  int StandardBlockDimensions[3];
  double RootSpacing[3];
  
  // Global origin is chosen by ignoring ghost layers.
  // All indexes will be positive.
  // If we complete the ghost layer on boundary blocks,
  // the ghost layer will still be positive.
  double GlobalOrigin[3];
  
  // Each level will have a grid to help find neighbors.
  vtkstd::vector<vtkAMRDualGridHelperLevel*> Levels;

  int EnableDegenerateCells;
  // Degenerate regions that span processes.  We keep them in a queue
  // to communicate and process all at once.
  vtkstd::vector<vtkAMRDualGridHelperDegenerateRegion> DegenerateRegionQueue;
  void ProcessDegenerateRegionQueue();
  void SendDegenerateRegionsFromQueue(int remoteProc, int localProc);
  void ReceiveDegenerateRegionsFromQueue(int remoteProc, int localProc);
  
  // For transitions between levels, degeneracy works well to create
  // and contour wedges and pyramids, but the volume fraction values
  // in the high-level blocks ghost cells need to be the same
  // as the closest cell in the low resolution block.  These methods
  // copy low values to high.
  void CopyDegenerateRegionBlockToBlock(int regionX, int regionY, int regionZ,
                                        vtkAMRDualGridHelperBlock* lowResBlock,
                                        vtkAMRDualGridHelperBlock* highResBlock);
  void* CopyDegenerateRegionBlockToMessage(
    int regionX, int regionY, int regionZ,
    vtkAMRDualGridHelperBlock* lowResBlock,
    vtkAMRDualGridHelperBlock* highResBlock,
    void* messagePtr);
  void* CopyDegenerateRegionMessageToBlock(
    int regionX, int regionY, int regionZ,
    vtkAMRDualGridHelperBlock* lowResBlock,
    vtkAMRDualGridHelperBlock* highResBlock,
    void* messagePtr);

private:
  vtkAMRDualGridHelper(const vtkAMRDualGridHelper&);  // Not implemented.
  void operator=(const vtkAMRDualGridHelper&);  // Not implemented.

//ETX
};


//BTX

// I need to define this small object in the namespace of this class.

//----------------------------------------------------------------------------
// Is there a better way of defining these in the namespace?
#define vtkAMRRegionBitOwner 128
// mask: The first 7 bits are going to store the degenerate level difference.
#define vtkAMRRegionBitsDegenerateMask 127
class vtkAMRDualGridHelperBlock
{
public:
  vtkAMRDualGridHelperBlock();
  ~vtkAMRDualGridHelperBlock();

  // We assume that all blocks have ghost levels and are the same
  // dimension.  The vtk spy reader strips the ghost cells on
  // boundary blocks (on the outer surface of the data set).
  // This method adds them back.
  void AddBackGhostLevels(int standardBlockDimensions[3]);

  int Level;
  
  // I am sick of looping over the grid to find the grid index
  // of the blocks.
  int GridIndex[3];
  
  // This is the global index of the origin of the image.
  // This is important since not all blocks have ghost layers on minimum side.
  int OriginIndex[3];
  
  // The process that has the actual data (image).
  int ProcessId;

  // If the block is local
  vtkImageData* Image;

  // How are we going to index faces.
  vtkAMRDualGridHelperFace* Faces[6];
  void SetFace(int faceId, vtkAMRDualGridHelperFace* face);

  // This is set when we have a copy of the image.
  // We need to modify the ghost layers of level interfaces.
  unsigned char CopyFlag;

  // We have to assign cells shared between blocks so only one
  // block will process them.  Faces, edges and corners have to be
  // considered separately (Extent does not work).
  // If the two blocks are from different levels, then the higher
  // level block always processes the cells.  The high res cells
  // are degenerate to mesh with low level neighbor cells.
  // Save the assignment in the neighbor bits.
  // The first bit means that this block processes these cells.
  // The second bit is set when the neighbor has a lower resolution
  // and points need to be placed in the lower grid.
  // 3x3x3 neighborhood to get face, edge and corner neighbors.
  unsigned char RegionBits[3][3][3];

  // Bits 1-6 are set if faces 1-6 are on boundary of dataset
  // and have no neighbors.
  unsigned char BoundaryBits;

private:
};

//----------------------------------------------------------------------------
// Material surface point in the face.  The point lies on an edge of the 
// dual grid and has a material fraction array.
class vtkAMRDualGridHelperSeed
{
public:
  vtkAMRDualGridHelperSeed();
  ~vtkAMRDualGridHelperSeed(){};

  // Level is supplied externally.
  // This is the index of the adjacent dual point in the object.
  int Index[3];
  int FragmentId;

private:
};

//----------------------------------------------------------------------------
// Neighbor interfaces (Faces) default to the lowest level (resolution) block.
// We do not actually care about the neighbor blocks yet.
class vtkAMRDualGridHelperFace
{
public:
  vtkAMRDualGridHelperFace();
  ~vtkAMRDualGridHelperFace();

  // Sets Level, Origin and Normal axis from the parent block.
  // Any block using the face will do as long as they have the same level.
  void InheritBlockValues(vtkAMRDualGridHelperBlock* block, int faceIndex);

  // This is the lowest level of the two sides of the face.
  int Level;
  // This is the index of the face origin.
  int OriginIndex[3];
  // 0 = X, 1 = Y, 2 = Z.
  int NormalAxis;

  // Sparse array of points for equivalence computation.
  vtkstd::vector<vtkAMRDualGridHelperSeed> FragmentIds;
  void AddFragmentSeed(int level, int x, int y, int z, int fragmentId);

  // This is the number of blocks pointing to this face.
  int UseCount;
  // Decrement UseCount and delete if it hit 0.
  void Unregister();

private:
};

//ETX

#endif
