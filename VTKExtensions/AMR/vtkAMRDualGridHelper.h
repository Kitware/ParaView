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
/**
 * @class   vtkAMRDualGridHelper
 * @brief   Tools for processing AMR as a dual grid.
 *
 * This helper object was developed to help the AMR dual grid connectivity
 * and integration filter but I also want a dual grid iso surface filter
 * so I mad it a separate class.  The API needs to be improved to make
 * it more generally useful.
 * This class will take advantage of some meta information, if available
 * from a coprocessing adaptor.  If not available, it will compute the
 * information.
*/

#ifndef vtkAMRDualGridHelper_h
#define vtkAMRDualGridHelper_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsAMRModule.h" //needed for exports
#include <map>
#include <vector>

class vtkDataArray;
class vtkIntArray;
class vtkIdTypeArray;
class vtkNonOverlappingAMR;
class vtkAMRDualGridHelperBlock;
class vtkAMRDualGridHelperLevel;
class vtkMultiProcessController;
class vtkImageData;
class vtkAMRDualGridHelperDegenerateRegion;
class vtkAMRDualGridHelperFace;
class vtkAMRDualGridHelperCommRequestList;

//----------------------------------------------------------------------------
class VTKPVVTKEXTENSIONSAMR_EXPORT vtkAMRDualGridHelper : public vtkObject
{
public:
  static vtkAMRDualGridHelper* New();
  vtkTypeMacro(vtkAMRDualGridHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * An option to turn off copying ghost values across process boundaries.
   * If the ghost values are already correct, then the extra communication is
   * not necessary.  If this assumption is wrong, this option will produce
   * cracks / seams.  This is off by default.
   */
  vtkGetMacro(SkipGhostCopy, int);
  vtkSetMacro(SkipGhostCopy, int);
  vtkBooleanMacro(SkipGhostCopy, int);
  //@}

  //@{
  /**
   * Turn on/off the ability to create meshing between levels in the grid.  This
   * is on by default.  Set this before you call initialize.
   */
  vtkGetMacro(EnableDegenerateCells, int);
  vtkSetMacro(EnableDegenerateCells, int);
  vtkBooleanMacro(EnableDegenerateCells, int);
  //@}

  //@{
  /**
   * When this option is on (the default) and a controller that supports
   * asynchronous communication (like MPI) is detected, use asynchronous
   * communication where appropriate.  This can prevent processes from blocking
   * while waiting for communication in other processes to finish.
   */
  vtkGetMacro(EnableAsynchronousCommunication, int);
  vtkSetMacro(EnableAsynchronousCommunication, int);
  vtkBooleanMacro(EnableAsynchronousCommunication, int);
  //@}

  //@{
  /**
   * The controller to use for communication.
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);
  //@}

  int Initialize(vtkNonOverlappingAMR* input);
  int SetupData(vtkNonOverlappingAMR* input, const char* arrayName);
  const double* GetGlobalOrigin() { return this->GlobalOrigin; }
  const double* GetRootSpacing() { return this->RootSpacing; }
  int GetNumberOfBlocks() { return this->NumberOfBlocksInThisProcess; }
  int GetNumberOfLevels() { return (int)(this->Levels.size()); }
  int GetNumberOfBlocksInLevel(int level);
  vtkAMRDualGridHelperBlock* GetBlock(int level, int blockIdx);
  vtkAMRDualGridHelperBlock* GetBlock(int level, int xGrid, int yGrid, int zGrid);

  /**
   * I am generalizing the code that copies lowres blocks to highres ghost regions.
   * I need to do this for the clip filter (level mask).

   * For transitions between levels, degeneracy works well to create
   * and contour wedges and pyramids, but the volume fraction values
   * in the high-level blocks ghost cells need to be the same
   * as the closest cell in the low resolution block.  These methods
   * copy low values to high.
   */
  void CopyDegenerateRegionBlockToBlock(int regionX, int regionY, int regionZ,
    vtkAMRDualGridHelperBlock* lowResBlock, vtkDataArray* lowResArray,
    vtkAMRDualGridHelperBlock* highResBlock, vtkDataArray* highResArray);
  /**
   * This queues up either a copy from a remote process to this process
   * or a copy from this process to a remote process.
   * Only the local block needs an array.  LowRes block is the source.
   */
  void QueueRegionRemoteCopy(int regionX, int regionY, int regionZ,
    vtkAMRDualGridHelperBlock* lowResBlock, vtkDataArray* lowResArray,
    vtkAMRDualGridHelperBlock* highResBlock, vtkDataArray* highResArray);
  /**
   * This should be called on every process.  It processes the queue of region copies.
   * It sends and copies the regions into blocks.
   */
  void ProcessRegionRemoteCopyQueue(bool hackLevelFlag);
  /**
   * Call this before adding regions to the queue.  It clears the queue.
   */
  void ClearRegionRemoteCopyQueue();
  //@{
  /**
   * It is convenient to get this here.
   */
  vtkGetStringMacro(ArrayName);
  //@}

private:
  vtkAMRDualGridHelper();
  ~vtkAMRDualGridHelper() override;

  char* ArrayName;
  int DataTypeSize;
  vtkSetStringMacro(ArrayName);

  // Distributed execution
  void ShareMetaData();
  void ShareBlocks();
  void ShareBlocksWithNeighbors(vtkIntArray* neighbors);
  void ShareBlocksWithNeighborsAsynchronous(vtkIntArray* neighbors);
  void ShareBlocksWithNeighborsSynchronous(vtkIntArray* neighbors);
  void MarshalBlocks(vtkIntArray* buffer);
  void UnmarshalBlocks(vtkIntArray* buffer);
  void UnmarshalBlocksFromOne(vtkIntArray* buffer, int blockProc);

  vtkMultiProcessController* Controller;
  void ComputeGlobalMetaData(vtkNonOverlappingAMR* input);
  void AddBlock(int level, int id, vtkImageData* volume);

  // Manage connectivity seeds between blocks.
  void CreateFaces();
  void FindExistingFaces(vtkAMRDualGridHelperBlock* block, int level, int x, int y, int z);

  // Assign shared cells between blocks and meshing
  // changes in level.
  void AssignSharedRegions();
  void AssignBlockSharedRegions(
    vtkAMRDualGridHelperBlock* block, int blockLevel, int blockX, int blockY, int blockZ);
  int ClaimBlockSharedRegion(vtkAMRDualGridHelperBlock* block, int blockX, int blockY, int blockZ,
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
  std::vector<vtkAMRDualGridHelperLevel*> Levels;

  int EnableDegenerateCells;

  void ProcessRegionRemoteCopyQueueSynchronous(bool hackLevelFlag);
  void SendDegenerateRegionsFromQueueSynchronous(int destProc, vtkIdType messageLength);
  void ReceiveDegenerateRegionsFromQueueSynchronous(
    int srcProc, vtkIdType messageLength, bool hackLevelFlag);

  // NOTE: These methods are NOT DEFINED if not compiled with MPI.
  void ProcessRegionRemoteCopyQueueMPIAsynchronous(bool hackLevelFlag);
  void SendDegenerateRegionsFromQueueMPIAsynchronous(
    int recvProc, vtkIdType messageLength, vtkAMRDualGridHelperCommRequestList& sendList);
  void ReceiveDegenerateRegionsFromQueueMPIAsynchronous(
    int sendProc, vtkIdType messageLength, vtkAMRDualGridHelperCommRequestList& receiveList);
  void FinishDegenerateRegionsCommMPIAsynchronous(bool hackLevelFlag,
    vtkAMRDualGridHelperCommRequestList& sendList,
    vtkAMRDualGridHelperCommRequestList& receiveList);

  // Degenerate regions that span processes.  We keep them in a queue
  // to communicate and process all at once.
  std::vector<vtkAMRDualGridHelperDegenerateRegion> DegenerateRegionQueue;
  void DegenerateRegionMessageSize(vtkIdTypeArray* srcProcs, vtkIdTypeArray* destProc);
  void* CopyDegenerateRegionBlockToMessage(
    const vtkAMRDualGridHelperDegenerateRegion& region, void* messagePtr);
  const void* CopyDegenerateRegionMessageToBlock(
    const vtkAMRDualGridHelperDegenerateRegion& region, const void* messagePtr, bool hackLevelFlag);
  void MarshalDegenerateRegionMessage(void* messagePtr, int destProc);
  void UnmarshalDegenerateRegionMessage(
    const void* messagePtr, int messageLength, int srcProc, bool hackLevelFlag);

  int SkipGhostCopy;

  int EnableAsynchronousCommunication;

private:
  vtkAMRDualGridHelper(const vtkAMRDualGridHelper&) = delete;
  void operator=(const vtkAMRDualGridHelper&) = delete;
};

// I need to define this small object in the namespace of this class.

//----------------------------------------------------------------------------
// Is there a better way of defining these in the namespace?
#define vtkAMRRegionBitOwner 128
// mask: The first 7 bits are going to store the degenerate level difference.
#define vtkAMRRegionBitsDegenerateMask 127
class VTKPVVTKEXTENSIONSAMR_EXPORT vtkAMRDualGridHelperBlock
{
public:
  vtkAMRDualGridHelperBlock();
  ~vtkAMRDualGridHelperBlock();

  void ResetRegionBits();
  // We assume that all blocks have ghost levels and are the same
  // dimension.  The vtk spy reader strips the ghost cells on
  // boundary blocks (on the outer surface of the data set).
  // This method adds them back.
  void AddBackGhostLevels(int standardBlockDimensions[3]);

  int Level;
  int BlockId;

  // I am sick of looping over the grid to find the grid index
  // of the blocks.
  int GridIndex[3];

  // This is the global index of the origin of the image.
  // This is important since not all blocks have ghost layers on minimum side.
  // It appears that this is the origin of the ghost pixel if the image has a ghost pixel.
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

  // Different algorithms need to store different information
  // with the blocks.  I could make this a vtkObject so the destructor
  // would delete it.
  void* UserData;

private:
};

//----------------------------------------------------------------------------
// Material surface point in the face.  The point lies on an edge of the
// dual grid and has a material fraction array.
class VTKPVVTKEXTENSIONSAMR_EXPORT vtkAMRDualGridHelperSeed
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
class VTKPVVTKEXTENSIONSAMR_EXPORT vtkAMRDualGridHelperFace
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
  std::vector<vtkAMRDualGridHelperSeed> FragmentIds;
  void AddFragmentSeed(int level, int x, int y, int z, int fragmentId);

  // This is the number of blocks pointing to this face.
  int UseCount;
  // Decrement UseCount and delete if it hit 0.
  void Unregister();

private:
};

#endif

// VTK-HeaderTest-Exclude: vtkAMRDualGridHelper.h
