/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDualClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRDualClip.h"
#include "vtkAMRDualGridHelper.h"

#include <vector>

// Pipeline & VTK
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMultiProcessController.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
// PV interface
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkMath.h"
// Data sets
#include "vtkAMRBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include <ctime>
#include <math.h>

vtkStandardNewMacro(vtkAMRDualClip);

vtkCxxSetObjectMacro(vtkAMRDualClip, Controller, vtkMultiProcessController);

// 1: Create meta data just like AMR iso.
// 2: Assign shared regions just like AMR iso.
// 3: Copy Ghost layers (low to high) just like AMR iso.
// New generate a decimation mask for each block
// This is similar to the locator in AMR iso.
// They can be created on demand like amr iso (not all at once).
// Center region of mask does not depend on neighbors.
// Ghost regions of mask must come from neighbors.
// Decimation mask takes care of transitions between levels.

// Remaining issues.
// Degenerate internal points can be outside iso surface.
// Degenerate mask not shared between processes.
// Locator not merging points between blocks when (degenerate) levels are different.

#include "vtkAMRDualClipTables.cxx"

//============================================================================
// Used separately for each block.
// Has two locators  one for dual points (AMR cells), and one
// for dual edges (AMR faces) This is the typical 3 edge per voxel
// lookup.  We do need to worry about degeneracy because corners can merge
// and edges can merge when the degenerate cell is a wedge.

// Shared points are copied to neighbors before a locator is deleted.
// Locators are created on demand and deleted right after a block is complete.

// Setup of the level mask.
// Centers are computed independently.
// Centers must be computed before anypoints are added.
// Centers must be computed before they are used to set ghost values in neighbors.
// Ghost values must be copied from neighbor before points are added.
// Centers are filled in when the locator is created.
// Ghost values are copied whena block is deleted and before a locator is used.
// I will have to keep flags to indicate which ghost regions have been set.

class vtkAMRDualClipLocator
{
public:
  vtkAMRDualClipLocator();
  ~vtkAMRDualClipLocator();

  // Description:
  // Dims are for the dual cells including ghost layers.
  // This is called multiple times to prepare for a new block.
  void Initialize(int xDualCellDim, int yDualCellDim, int zDualCellDim);

  // Description:
  // Lookup and setting uses this pointer. Using a pointer keeps
  // the contour filter from having to lookup a point and second
  // adding a point (both are very similar).
  // The edge index uses VTK voxel edge indexing scheme.
  vtkIdType* GetEdgePointer(int xCell, int yCell, int zCell, int edgeIdx);

  // Description:
  // Same but for corners not edges.  This uses my binary indexing of corners.
  // 0:(000) 1:(100) 2:(010) 3:(110) 4:(001) 5:(101)....
  vtkIdType* GetCornerPointer(int xCell, int yCell, int zCell, int cornerIdx, int blockOrigin[3]);

  // Description:
  // To handle degenerate cells, indicate the level difference between the block
  // and region neighbor.
  // void CopyRegionLevelDifferences(vtkAMRDualGridHelperBlock* block);

  // Description:
  // Deprecciated
  // Used to share point ids between block locators.
  void SharePointIdsWithNeighbor(vtkAMRDualClipLocator* neighborLocator, int rx, int ry, int rz);

  void ShareBlockLocatorWithNeighbor(
    vtkAMRDualGridHelperBlock* block, vtkAMRDualGridHelperBlock* neighbor);

  // The level mask could be a separate object, but it is used
  // by the locator to position points.
  // This computes just the center region.
  // Other regions need to be copied.
  // If the center has already been computed, then this returns immediately
  // (so it is ok to call this multiple times).
  void ComputeLevelMask(vtkDataArray* scalars, double isoValue, int decimate);

  // This is used to synchronize the ghost level mask with neighbors.
  void CopyNeighborLevelMask(
    vtkAMRDualGridHelperBlock* myBlock, vtkAMRDualGridHelperBlock* neighborBlock);

  // Used to set the level mask of capped faces.
  void CapLevelMaskFace(int axis, int face);

  // Access to the level mask for any cell.
  unsigned char GetLevelMaskValue(int x, int y, int z);

  vtkUnsignedCharArray* GetLevelMaskArray() { return this->LevelMaskArray; }

private:
  int DualCellDimensions[3];
  // Increments for translating 3d to 1d.  XIncrement = 1;
  int YIncrement;
  int ZIncrement;
  int ArrayLength;
  // I am just going to use 3 separate arrays for edges on the 3 axes.
  vtkIdType* XEdges;
  vtkIdType* YEdges;
  vtkIdType* ZEdges;
  vtkIdType* Corners;

  // Level mask indicated the placement of dual points (centers) for each
  // cell.  This is used for both degenerate level transitions between
  // blocks and decimating the interior of blocks.  The level value indicates
  // which level grid the point will be on.  If all 8 cells (children of a
  // cell in the next lower level) are interior, then all eight merge to the
  // same point.
  // unsigned char* LevelMask;
  vtkUnsignedCharArray* LevelMaskArray;
  unsigned char* GetLevelMaskPointer();
  void RecursiveComputeLevelMask(int depth);

  // Flag to indicate that center region has been initialized.
  unsigned char CenterLevelMaskComputed;
};

//----------------------------------------------------------------------------
unsigned char* vtkAMRDualClipLocator::GetLevelMaskPointer()
{
  if (this->LevelMaskArray == 0)
  {
    return 0;
  }
  return this->LevelMaskArray->GetPointer(0);
}

//----------------------------------------------------------------------------
vtkAMRDualClipLocator* vtkAMRDualClipGetBlockLocator(vtkAMRDualGridHelperBlock* block)
{
  if (block->UserData == 0)
  {
    vtkImageData* image = block->Image;
    if (image == 0)
    { // Remote blocks are only to setup local block bit flags.
      // They do not need locators.
      return 0;
    }
    int extent[6];
    // This is the same as the cell extent of the original grid (with ghosts).
    image->GetExtent(extent);
    --extent[1];
    --extent[3];
    --extent[5];

    vtkAMRDualClipLocator* locator = new vtkAMRDualClipLocator;
    block->UserData = (void*)(locator); // Block owns it now (and will delete it).
    locator->Initialize(extent[1] - extent[0], extent[3] - extent[2], extent[5] - extent[4]);

    return locator;
  }
  return (vtkAMRDualClipLocator*)(block->UserData);
}

//----------------------------------------------------------------------------
// The only data specific stuff we need to do for the contour.
template <class T>
void vtkDualGridClipInitializeLevelMask(
  T* scalarPtr, double isoValue, unsigned char* levelMask, int dims[3])
{
  // unsigned char flag = 1;

  // We only set the inside because the ghost regions can already be set.
  scalarPtr += 1 + dims[0] + dims[0] * dims[1];
  levelMask += 1 + dims[0] + dims[0] * dims[1];
  // Start with two because skipping from and back ghost.
  // The exact value of zz does not matter.
  // This is easier than comparing < dim-1.
  for (int zz = 2; zz < dims[2]; ++zz)
  {
    for (int yy = 2; yy < dims[1]; ++yy)
    {
      for (int xx = 2; xx < dims[0]; ++xx)
      {
        // Lets do relative levels (to block) / level diff.
        // Then we do not need the block level.  The only trouble is that
        // we need to offset by 1 so that 0 can be special value (outside).
        if (*scalarPtr++ > isoValue)
        {
          *levelMask++ = 1;
        }
        else
        { // Special value  indicating point is outside clipped volume.
          *levelMask++ = 0;
          // flag = 0;
        }
      }
      // Skip last ghost of this row and first ghost of next.
      levelMask += 2;
      scalarPtr += 2;
    }
    // Skip last ghost row of this plane and first ghost row of next.
    levelMask += 2 * dims[0];
    scalarPtr += 2 * dims[0];
  }
}

//----------------------------------------------------------------------------
// Initializes the center region of the level mask.
// The other regions are set to default values, but have to be copied
// from neighbors if they are not on the boundary of the dataset.
void vtkAMRDualClipLocator::ComputeLevelMask(vtkDataArray* scalars, double isoValue, int decimate)
{
  if (this->CenterLevelMaskComputed)
  {
    return;
  }
  this->CenterLevelMaskComputed = 1;
  int dims[3];
  dims[0] = this->DualCellDimensions[0] + 1;
  dims[1] = this->DualCellDimensions[1] + 1;
  dims[2] = this->DualCellDimensions[2] + 1;

  switch (scalars->GetDataType())
  {
    vtkTemplateMacro(vtkDualGridClipInitializeLevelMask(
      (VTK_TT*)(scalars->GetVoidPointer(0)), isoValue, this->GetLevelMaskPointer(), dims));
    default:
      vtkGenericWarningMacro("Execute: Unknown ScalarType");
  }

  // Reduce point levels based on cell neighbors.
  // If all the high level cells in a low level cell have the same value
  // then reduce the level of all.
  // Do this only for center region because ghost values need to be obtainied
  // from neighbors.
  // We might add a restriction that level cannot change more than 1 brterrnneighboirs
  // but this would make computation less local and maybe require all locators to be compted
  // at once (or revised?).

  // Recursive : Going in compute tree,  going out fill it mask.

  // If decimation is off, then we might be able to skip computing the level mask.
  // It might avoid some communication when synchronizing masks.
  // This is easiest for now.
  if (decimate)
  {
    this->RecursiveComputeLevelMask(0);
  }
}

//----------------------------------------------------------------------------
// face = 0 => min, face = 1 => max
void vtkAMRDualClipLocator::CapLevelMaskFace(int axis, int face)
{
  unsigned char* startPtr;
  int normalInc;
  int iiInc, jjInc;
  int iiMax, jjMax;

  iiMax = jjMax = iiInc = jjInc = normalInc = 0;
  startPtr = this->GetLevelMaskPointer();
  switch (axis)
  {
    case 0:
      normalInc = 1;
      iiInc = this->ZIncrement;
      jjInc = this->YIncrement;
      iiMax = this->DualCellDimensions[2];
      jjMax = this->DualCellDimensions[1];
      break;
    case 1:
      normalInc = this->YIncrement;
      iiInc = this->ZIncrement;
      jjInc = 1;
      iiMax = this->DualCellDimensions[2];
      jjMax = this->DualCellDimensions[0];
      break;
    case 2:
      normalInc = this->ZIncrement;
      iiInc = this->YIncrement;
      jjInc = 1;
      iiMax = this->DualCellDimensions[1];
      jjMax = this->DualCellDimensions[0];
      break;
    default:
      vtkGenericWarningMacro("Bad axis.");
  }
  // Handle the max face cases.
  if (face == 1)
  {
    startPtr = startPtr + this->ArrayLength - 1;
    normalInc = -normalInc;
    iiInc = -iiInc;
    jjInc = -jjInc;
  }

  // Copy to ghost regions because of capping surfaces.
  // Ghost values that do not overlap neighbor blocks
  // (on dataset boundary) need values.
  // Just copy nearest internal block value.
  unsigned char *iiPtr, *jjPtr;
  iiPtr = startPtr;
  for (int ii = 0; ii <= iiMax; ++ii)
  {
    jjPtr = iiPtr;
    for (int jj = 0; jj <= jjMax; ++jj)
    {
      *jjPtr = jjPtr[normalInc];
      jjPtr += jjInc;
    }
    iiPtr += iiInc;
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualClipLocator::RecursiveComputeLevelMask(int depth)
{
  // Recursion depth is small so stack size is not an issue.
  int xInc = (1 << depth);
  int yInc = this->YIncrement << depth;
  int zInc = this->ZIncrement << depth;
  int xyInc = xInc + yInc;
  int xzInc = xInc + zInc;
  int yzInc = yInc + zInc;
  int xyzInc = xInc + yInc + zInc;
  int xMax = (this->DualCellDimensions[0] - 1);
  int yMax = (this->DualCellDimensions[1] - 1);
  int zMax = (this->DualCellDimensions[2] - 1);
  // Teminate when we run out of factors of two.
  if (xMax & (1 << depth))
  {
    return;
  }
  if (yMax & (1 << depth))
  {
    return;
  }
  if (zMax & (1 << depth))
  {
    return;
  }
  ++depth;
  xMax = xMax >> depth;
  yMax = yMax >> depth;
  zMax = zMax >> depth;

  // Compute the next level of the tree
  unsigned char *xPtr, *yPtr, *zPtr;
  // Skip the ghost regions.
  zPtr = this->GetLevelMaskPointer() + 1 + this->YIncrement + this->ZIncrement;
  for (int zz = 0; zz < zMax; ++zz)
  {
    yPtr = zPtr;
    for (int yy = 0; yy < yMax; ++yy)
    {
      xPtr = yPtr;
      for (int xx = 0; xx < xMax; ++xx)
      {
        if (xPtr[0] == depth && xPtr[xInc] == depth && xPtr[yInc] == depth && xPtr[zInc] == depth &&
          xPtr[xyInc] == depth && xPtr[xzInc] == depth && xPtr[yzInc] == depth &&
          xPtr[xyzInc] == depth)
        {
          ++xPtr[0];
        }
        xPtr += (xInc << 1);
      }
      yPtr += (yInc << 1);
    }
    zPtr += (zInc << 1);
  }

  this->RecursiveComputeLevelMask(depth);

  // Now fill in the blocks.
  // I do this last because the recursive call
  // may change the level rocorded inthe corner.
  unsigned char *xPtr2, *yPtr2, *zPtr2;
  int xMax2 = 1 << depth;
  int yMax2 = 1 << depth;
  int zMax2 = 1 << depth;
  zPtr = this->GetLevelMaskPointer() + 1 + this->YIncrement + this->ZIncrement;
  ++depth;
  for (int zz = 0; zz < zMax; ++zz)
  {
    yPtr = zPtr;
    for (int yy = 0; yy < yMax; ++yy)
    {
      xPtr = yPtr;
      for (int xx = 0; xx < xMax; ++xx)
      {
        if (xPtr[0] == depth)
        {
          // Fill in the block
          zPtr2 = xPtr;
          for (int z2 = 0; z2 < zMax2; ++z2)
          {
            yPtr2 = zPtr2;
            for (int y2 = 0; y2 < yMax2; ++y2)
            {
              xPtr2 = yPtr2;
              for (int x2 = 0; x2 < xMax2; ++x2)
              {
                *xPtr2++ = depth;
              }
              yPtr2 += this->YIncrement;
            }
            zPtr2 += this->ZIncrement;
          }
        }
        xPtr += (xInc << 1);
      }
      yPtr += (yInc << 1);
    }
    zPtr += (zInc << 1);
  }
}

//----------------------------------------------------------------------------
// Caller needs to make sure the source has computed the level mask.
// I am not sure of the difference between CopyNeighborLevelMask and .....
void vtkAMRDualClipLocator::CopyNeighborLevelMask(
  vtkAMRDualGridHelperBlock* myBlock, vtkAMRDualGridHelperBlock* neighborBlock)
{
  // We never have to copy from a higher level to a lower level.
  // the higher level block always handles the shared region.
  // Neighbor is the source,  this is the destination.
  // I will put a check in the caller so that only the block that owns
  // this region calls this method.
  if (neighborBlock->Level > myBlock->Level)
  {
    return;
  }
  vtkAMRDualClipLocator* neighborLocator = vtkAMRDualClipGetBlockLocator(neighborBlock);
  if (neighborLocator == 0)
  { // Figuring out logic for parallel case.
    return;
  }

  // We copy from the center region to ghost region.

  // Compute the intersection in the high level destination block
  int sourceExt[6]; // Center region
  sourceExt[0] = neighborBlock->OriginIndex[0] + 1;
  sourceExt[2] = neighborBlock->OriginIndex[1] + 1;
  sourceExt[4] = neighborBlock->OriginIndex[2] + 1;
  sourceExt[1] = sourceExt[0] + neighborLocator->DualCellDimensions[0] - 2;
  sourceExt[3] = sourceExt[2] + neighborLocator->DualCellDimensions[1] - 2;
  sourceExt[5] = sourceExt[4] + neighborLocator->DualCellDimensions[2] - 2;
  int destExt[6]; // all regions (including ghosts)
  destExt[0] = myBlock->OriginIndex[0];
  destExt[2] = myBlock->OriginIndex[1];
  destExt[4] = myBlock->OriginIndex[2];
  destExt[1] = destExt[0] + this->DualCellDimensions[0];
  destExt[3] = destExt[2] + this->DualCellDimensions[1];
  destExt[5] = destExt[4] + this->DualCellDimensions[2];

  // Convert the source extent to the destination coordinates.
  // All extents have already been shifted to be positive.
  int levelDiff = myBlock->Level - neighborBlock->Level;
  sourceExt[0] = (sourceExt[0] << levelDiff);
  sourceExt[1] = ((sourceExt[1] + 1) << levelDiff) - 1;
  sourceExt[2] = (sourceExt[2] << levelDiff);
  sourceExt[3] = ((sourceExt[3] + 1) << levelDiff) - 1;
  sourceExt[4] = (sourceExt[4] << levelDiff);
  sourceExt[5] = ((sourceExt[5] + 1) << levelDiff) - 1;

  // Take the intersection to find the destination extent.
  if (destExt[0] < sourceExt[0])
  {
    destExt[0] = sourceExt[0];
  }
  if (destExt[1] > sourceExt[1])
  {
    destExt[1] = sourceExt[1];
  }
  if (destExt[2] < sourceExt[2])
  {
    destExt[2] = sourceExt[2];
  }
  if (destExt[3] > sourceExt[3])
  {
    destExt[3] = sourceExt[3];
  }
  if (destExt[4] < sourceExt[4])
  {
    destExt[4] = sourceExt[4];
  }
  if (destExt[5] > sourceExt[5])
  {
    destExt[5] = sourceExt[5];
  }

  // Loop over the extent.
  unsigned char* sourcePtr = neighborLocator->GetLevelMaskPointer();
  unsigned char* destPtr = this->GetLevelMaskPointer();
  // +1 is for ghost offset.
  destPtr += (destExt[0] - myBlock->OriginIndex[0]);
  destPtr += (destExt[2] - myBlock->OriginIndex[1]) * this->YIncrement;
  destPtr += (destExt[4] - myBlock->OriginIndex[2]) * this->ZIncrement;
  unsigned char *xPtr, *yPtr, *zPtr;
  zPtr = destPtr;
  int sx, sy, sz;
  for (int zz = destExt[4]; zz <= destExt[5]; ++zz)
  {
    sz = (zz >> levelDiff) - neighborBlock->OriginIndex[2];
    yPtr = zPtr;
    for (int yy = destExt[2]; yy <= destExt[3]; ++yy)
    {
      sy = (yy >> levelDiff) - neighborBlock->OriginIndex[1];
      xPtr = yPtr;
      for (int xx = destExt[0]; xx <= destExt[1]; ++xx)
      {
        // Compute the source for this pixel.
        sx = (xx >> levelDiff) - neighborBlock->OriginIndex[0];
        *xPtr = sourcePtr[sx + sy * this->YIncrement + sz * this->ZIncrement] + levelDiff;
        ++xPtr;
      }
      yPtr += this->YIncrement;
    }
    zPtr += this->ZIncrement;
  }
}

//----------------------------------------------------------------------------
unsigned char vtkAMRDualClipLocator::GetLevelMaskValue(int x, int y, int z)
{
  unsigned char* ptr = this->GetLevelMaskPointer();

  if (ptr[x + (y * this->YIncrement) + (z * this->ZIncrement)] != 1)
  {
    // cerr << "debug\n";
  }

  return ptr[x + (y * this->YIncrement) + (z * this->ZIncrement)];
}

//----------------------------------------------------------------------------
vtkAMRDualClipLocator::vtkAMRDualClipLocator()
{
  this->YIncrement = this->ZIncrement = 0;
  this->ArrayLength = 0;
  this->XEdges = this->YEdges = this->ZEdges = 0;
  this->Corners = 0;
  for (int ii = 0; ii < 3; ++ii)
  {
    this->DualCellDimensions[ii] = 0;
  }
  this->LevelMaskArray = 0;
  this->CenterLevelMaskComputed = 0;
}
//----------------------------------------------------------------------------
vtkAMRDualClipLocator::~vtkAMRDualClipLocator()
{
  this->Initialize(0, 0, 0);
}
//----------------------------------------------------------------------------
void vtkAMRDualClipLocator::Initialize(int xDualCellDim, int yDualCellDim, int zDualCellDim)
{
  if (xDualCellDim != this->DualCellDimensions[0] || yDualCellDim != this->DualCellDimensions[1] ||
    zDualCellDim != this->DualCellDimensions[2])
  {
    if (this->XEdges)
    { // They are all allocated at once, so separate checks are not necessary.
      delete[] this->XEdges;
      delete[] this->YEdges;
      delete[] this->ZEdges;
      delete[] this->Corners;
      this->LevelMaskArray->Delete();
      this->LevelMaskArray = 0;
    }
    if (xDualCellDim > 0 && yDualCellDim > 0 && zDualCellDim > 0)
    {
      this->DualCellDimensions[0] = xDualCellDim;
      this->DualCellDimensions[1] = yDualCellDim;
      this->DualCellDimensions[2] = zDualCellDim;
      // We have to increase dimensions by one to capture edges on the max faces.
      this->YIncrement = this->DualCellDimensions[0] + 1;
      this->ZIncrement = this->YIncrement * (this->DualCellDimensions[1] + 1);
      this->ArrayLength = this->ZIncrement * (this->DualCellDimensions[2] + 1);
      this->XEdges = new vtkIdType[this->ArrayLength];
      this->YEdges = new vtkIdType[this->ArrayLength];
      this->ZEdges = new vtkIdType[this->ArrayLength];
      this->Corners = new vtkIdType[this->ArrayLength];
      this->LevelMaskArray = vtkUnsignedCharArray::New();
      this->LevelMaskArray->SetNumberOfTuples(this->ArrayLength);
      // 255 is a special value that means the pixel is uninitialized.
      memset(this->GetLevelMaskPointer(), 255, this->ArrayLength);
    }
    else
    {
      this->YIncrement = this->ZIncrement = 0;
      this->ArrayLength = 0;
      this->DualCellDimensions[0] = 0;
      this->DualCellDimensions[1] = 0;
      this->DualCellDimensions[2] = 0;
    }
  }

  for (int idx = 0; idx < this->ArrayLength; ++idx)
  {
    this->XEdges[idx] = this->YEdges[idx] = this->ZEdges[idx] = -1;
    this->Corners[idx] = -1;
  }
}

//----------------------------------------------------------------------------
// No bounds checking.
// I am going to move points that are very close to a corner to a corner
// I assume this will improve the mesh.
vtkIdType* vtkAMRDualClipLocator::GetEdgePointer(int xCell, int yCell, int zCell, int edgeIdx)
{
  // In the past, I move edge points to corner points when they were close,
  // but predictable, this cause non-manifold edges to occur.

  // I assume VTK edge index and binary corner index.
  int ptIdx0 = vtkAMRDualIsoEdgeToPointsTable[edgeIdx][0];
  int ptIdx1 = vtkAMRDualIsoEdgeToPointsTable[edgeIdx][1];
  // Use bitwise exclusive or to get edge axis.
  int axis = (ptIdx0 ^ ptIdx1);
  // Some fancy bit logic to increment cell index based on edge here.
  // Bitwise exclusive-or to mask the edge axis.
  // ptIdx0 = (ptIdx0 ^ axis); // This had a flaw.
  ptIdx0 = (ptIdx0 & ptIdx1);
  // Adjusted index to 3 axis template.
  int xp0 = xCell;
  int yp0 = yCell;
  int zp0 = zCell;
  if (ptIdx0 & 1)
  {
    ++xp0;
  }
  if (ptIdx0 & 2)
  {
    ++yp0;
  }
  if (ptIdx0 & 4)
  {
    ++zp0;
  }

  switch (axis)
  {
    case 1:
    {
      return this->XEdges + (xp0 + (yp0 * this->YIncrement) + (zp0 * this->ZIncrement));
    }
    case 2:
    {
      return this->YEdges + (xp0 + (yp0 * this->YIncrement) + (zp0 * this->ZIncrement));
    }
    case 4:
    {
      return this->ZEdges + (xp0 + (yp0 * this->YIncrement) + (zp0 * this->ZIncrement));
    }
    default:
      assert(0 && "Invalid edge index.");
      return 0;
  }
}

//----------------------------------------------------------------------------
// No bounds checking.
// We need to know the origin of the block
// because we have to know where degenerate boundaries are.
// This is only important in the rare cases when degenerate delta level is
// larger than 3 (degenerate block is in ghost region and is larger the
// whole block).  We could keep recursing past block boundaries when computing
// the level mask, but I do not think it would buy us much.
vtkIdType* vtkAMRDualClipLocator::GetCornerPointer(
  int xCell, int yCell, int zCell, int cornerIdx, int blockOrigin[3])
{
  int diff;

  // Compute the dual corner index from the dual cell index and corner id.
  xCell += cornerIdx & 1;
  yCell += (cornerIdx & 2) >> 1;
  zCell += (cornerIdx & 4) >> 2;

  // Find out the delta level degeneracy for this region.
  diff =
    this->GetLevelMaskPointer()[xCell + (yCell * this->YIncrement) + (zCell * this->ZIncrement)];
  --diff;
  // Short circuit for debugging.
  // This will not merge any point based on the level mask.
  // diff = 0;

  if (diff > 0)
  {
    // We have to modify the dual point index to reflect degeneracy.
    // The problem is that the range may become larger than our locator array.
    // The minimum extent can get smaller when we mask bits off.
    // Also, we have to convert back to relative index to remove the global offset.
    // Different point in the locator may be in different degenerate levels,
    // so we do need to convert index back to the original level.
    // It looks like we need to know the origin of the block.
    xCell += blockOrigin[0];
    xCell = ((xCell >> diff) << diff) - blockOrigin[0];
    if (xCell < 0)
    {
      xCell = 0;
    }
    yCell += blockOrigin[1];
    yCell = ((yCell >> diff) << diff) - blockOrigin[1];
    if (yCell < 0)
    {
      yCell = 0;
    }
    zCell += blockOrigin[2];
    zCell = ((zCell >> diff) << diff) - blockOrigin[2];
    if (zCell < 0)
    {
      zCell = 0;
    }
  }

  return this->Corners + (xCell + (yCell * this->YIncrement) + (zCell * this->ZIncrement));
}

//----------------------------------------------------------------------------
// Deprecciated
void vtkAMRDualClipLocator::SharePointIdsWithNeighbor(
  vtkAMRDualClipLocator* neighborLocator, int rx, int ry, int rz)
{
  int outMinX = 0;
  int outMinY = 0;
  int outMinZ = 0;
  // Compute the extent of the locator to copy.
  int ext[6];
  // Copy all possible overlap.
  // Moving too many will not hurt.
  ext[0] = 0;
  ext[1] = this->DualCellDimensions[0];
  if (rx == -1)
  {
    ext[1] = 1;
    outMinX = this->DualCellDimensions[0] - 1;
  }
  else if (rx == 1)
  {
    ext[0] = this->DualCellDimensions[0] - 1;
  }
  ext[2] = 0;
  ext[3] = this->DualCellDimensions[1];
  if (ry == -1)
  {
    ext[3] = 1;
    outMinY = this->DualCellDimensions[1] - 1;
  }
  else if (ry == 1)
  {
    ext[2] = this->DualCellDimensions[1] - 1;
  }
  ext[4] = 0;
  ext[5] = this->DualCellDimensions[2];
  if (rz == -1)
  {
    ext[5] = 1;
    outMinZ = this->DualCellDimensions[2] - 1;
  }
  else if (rz == 1)
  {
    ext[4] = this->DualCellDimensions[2] - 1;
  }

  vtkIdType pointId;
  int inOffsetZ = ext[0] + ext[2] * this->YIncrement + ext[4] * this->ZIncrement;
  int inOffsetY, inOffsetX;
  int outOffsetZ = outMinX + outMinY * this->YIncrement + outMinZ * this->ZIncrement;
  int outOffsetY, outOffsetX;
  for (int z = ext[4]; z <= ext[5]; ++z)
  {
    inOffsetY = inOffsetZ;
    outOffsetY = outOffsetZ;
    for (int y = ext[2]; y <= ext[3]; ++y)
    {
      inOffsetX = inOffsetY;
      outOffsetX = outOffsetY;
      for (int x = ext[0]; x <= ext[1]; ++x)
      {
        pointId = this->XEdges[inOffsetX];
        if (pointId > 0)
        {
          neighborLocator->XEdges[outOffsetX] = pointId;
        }
        pointId = this->YEdges[inOffsetX];
        if (pointId > 0)
        {
          neighborLocator->YEdges[outOffsetX] = pointId;
        }
        pointId = this->ZEdges[inOffsetX];
        if (pointId > 0)
        {
          neighborLocator->ZEdges[outOffsetX] = pointId;
        }
        pointId = this->Corners[inOffsetX];
        if (pointId > 0)
        {
          neighborLocator->Corners[outOffsetX] = pointId;
        }

        inOffsetX += 1;
        outOffsetX += 1;
      }
      inOffsetY += this->YIncrement;
      outOffsetY += this->YIncrement;
    }
    inOffsetZ += this->ZIncrement;
    outOffsetZ += this->ZIncrement;
  }
}

//----------------------------------------------------------------------------
// This version works with higher level neighbor blocks.
// Move the points on boundaries to neighbor locator so there will
// not be duplicate coincident points between blocks.
void vtkAMRDualClipLocator::ShareBlockLocatorWithNeighbor(
  vtkAMRDualGridHelperBlock* block, vtkAMRDualGridHelperBlock* neighbor)
{
  vtkAMRDualClipLocator* blockLocator = vtkAMRDualClipGetBlockLocator(block);
  vtkAMRDualClipLocator* neighborLocator = vtkAMRDualClipGetBlockLocator(neighbor);

  // Working on the logic to parallelize level mask.
  if (blockLocator == 0 || neighborLocator == 0)
  { // This occurs if the block is owned by a different process.
    return;
  }

  // Compute the extent of the locator to copy.
  // Moving too many will not hurt, so do not worry about which block owns the region.
  // Start with the extent of the neighbor.  It is higher level.
  int ext[6];

  // Copy all possible overlap.
  ext[0] = 0;
  ext[1] = neighborLocator->DualCellDimensions[0];
  ext[2] = 0;
  ext[3] = neighborLocator->DualCellDimensions[1];
  ext[4] = 0;
  ext[5] = neighborLocator->DualCellDimensions[2];

  // Now we need to convert the receiving low level block extent to the
  // source high level block extent.
  // Start with the out high level block.
  ext[0] += neighbor->OriginIndex[0];
  ext[1] += neighbor->OriginIndex[0];
  ext[2] += neighbor->OriginIndex[1];
  ext[3] += neighbor->OriginIndex[1];
  ext[4] += neighbor->OriginIndex[2];
  ext[5] += neighbor->OriginIndex[2];
  // Convert to the in low level block index coordinate system.
  int levelDiff = neighbor->Level - block->Level;
  ext[0] = (ext[0] >> levelDiff) - block->OriginIndex[0];
  ext[1] = (ext[1] >> levelDiff) - block->OriginIndex[0];
  ext[2] = (ext[2] >> levelDiff) - block->OriginIndex[1];
  ext[3] = (ext[3] >> levelDiff) - block->OriginIndex[1];
  ext[4] = (ext[4] >> levelDiff) - block->OriginIndex[2];
  ext[5] = (ext[5] >> levelDiff) - block->OriginIndex[2];
  // Intersect with in (source) low level block.
  if (ext[0] < 0)
  {
    ext[0] = 0;
  }
  if (ext[0] > blockLocator->DualCellDimensions[0])
  {
    ext[0] = blockLocator->DualCellDimensions[0];
  }
  if (ext[1] < 0)
  {
    ext[1] = 0;
  }
  if (ext[1] > blockLocator->DualCellDimensions[0])
  {
    ext[1] = blockLocator->DualCellDimensions[0];
  }
  if (ext[2] < 0)
  {
    ext[2] = 0;
  }
  if (ext[2] > blockLocator->DualCellDimensions[1])
  {
    ext[2] = blockLocator->DualCellDimensions[1];
  }
  if (ext[3] < 0)
  {
    ext[3] = 0;
  }
  if (ext[3] > blockLocator->DualCellDimensions[1])
  {
    ext[3] = blockLocator->DualCellDimensions[1];
  }
  if (ext[4] < 0)
  {
    ext[4] = 0;
  }
  if (ext[4] > blockLocator->DualCellDimensions[2])
  {
    ext[4] = blockLocator->DualCellDimensions[2];
  }
  if (ext[5] < 0)
  {
    ext[5] = 0;
  }
  if (ext[5] > blockLocator->DualCellDimensions[2])
  {
    ext[5] = blockLocator->DualCellDimensions[2];
  }

  vtkIdType pointId;
  int xOut, yOut, zOut;
  int inOffsetZ, inOffsetY, inOffsetX, outOffsetX, outOffsetY, outOffsetZ;
  inOffsetZ = ext[0] + ext[2] * blockLocator->YIncrement + ext[4] * blockLocator->ZIncrement;
  for (int zIn = ext[4]; zIn <= ext[5]; ++zIn)
  {
    inOffsetY = inOffsetZ;
    // Compute the output index.
    // Like the other places this locator indexconversion is done,
    // The min ghost index is shifted to fit into the locator array.
    zOut = ((zIn + block->OriginIndex[2]) << levelDiff) - neighbor->OriginIndex[2];
    if (zOut < 0)
    {
      zOut = 0;
    }
    outOffsetZ = zOut * neighborLocator->ZIncrement;
    for (int yIn = ext[2]; yIn <= ext[3]; ++yIn)
    {
      inOffsetX = inOffsetY;
      yOut = ((yIn + block->OriginIndex[1]) << levelDiff) - neighbor->OriginIndex[1];
      if (yOut < 0)
      {
        yOut = 0;
      }
      outOffsetY = outOffsetZ + yOut * neighborLocator->YIncrement;
      for (int xIn = ext[0]; xIn <= ext[1]; ++xIn)
      {
        xOut = ((xIn + block->OriginIndex[0]) << levelDiff) - neighbor->OriginIndex[0];
        if (xOut < 0)
        {
          xOut = 0;
        }
        outOffsetX = outOffsetY + xOut;

        pointId = blockLocator->XEdges[inOffsetX];
        if (pointId >= 0)
        {
          neighborLocator->XEdges[outOffsetX] = pointId;
        }
        pointId = blockLocator->YEdges[inOffsetX];
        if (pointId >= 0)
        {
          neighborLocator->YEdges[outOffsetX] = pointId;
        }
        pointId = blockLocator->ZEdges[inOffsetX];
        if (pointId >= 0)
        {
          neighborLocator->ZEdges[outOffsetX] = pointId;
        }
        pointId = blockLocator->Corners[inOffsetX];
        if (pointId >= 0)
        {
          neighborLocator->Corners[outOffsetX] = pointId;
        }

        inOffsetX += 1;
      }
      inOffsetY += blockLocator->YIncrement;
    }
    inOffsetZ += blockLocator->ZIncrement;
  }
}

//============================================================================
//----------------------------------------------------------------------------
// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkAMRDualClip::vtkAMRDualClip()
{
  this->IsoValue = 100.0;

  this->EnableInternalDecimation = 0;
  // When this is off, there
  this->EnableDegenerateCells = 1;
  this->EnableMultiProcessCommunication = 0;
  this->EnableMergePoints = 0;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  // Pipeline
  this->SetNumberOfOutputPorts(1);

  this->LevelMaskPointArray = 0;
  this->BlockIdCellArray = 0;
  this->Helper = 0;

  this->BlockLocator = 0;
}

//----------------------------------------------------------------------------
vtkAMRDualClip::~vtkAMRDualClip()
{
  if (this->BlockLocator)
  {
    delete this->BlockLocator;
    this->BlockLocator = 0;
  }
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkAMRDualClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IsoValue: " << this->IsoValue << endl;
  os << indent << "EnableInternalDecimation: " << this->EnableInternalDecimation << endl;
  os << indent << "EnableDegenerateCells: " << this->EnableDegenerateCells << endl;
  os << indent << "EnableMergePoints: " << this->EnableMergePoints << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//----------------------------------------------------------------------------
int vtkAMRDualClip::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRDualClip::FillOutputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      assert(0 && "Invalid output port.");
      break;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRDualClip::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the data set which we are to process
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* hbdsInput =
    vtkNonOverlappingAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the outputs
  // 0
  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (hbdsInput == 0)
  {
    // Do not deal with rectilinear grid
    vtkErrorMacro("This filter requires a vtkNonOverlappingAMR on its input.");
    return 0;
  }

  // This is a lot to go through to get the name of the array to process.
  vtkInformationVector* inArrayVec = this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    vtkErrorMacro("Problem finding array to process");
    return 0;
  }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  if (!inArrayInfo)
  {
    vtkErrorMacro("Problem getting name of array to process.");
    return 0;
  }
  if (!inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
  {
    vtkErrorMacro("Missing field name.");
    return 0;
  }
  const char* arrayNameToProcess = inArrayInfo->Get(vtkDataObject::FIELD_NAME());

  vtkMultiBlockDataSet* out = this->DoRequestData(hbdsInput, arrayNameToProcess);

  if (out)
  {
    mbdsOutput0->ShallowCopy(out);
    out->Delete();
  }
  else
  {
    return 0;
  }

  return 1;
}

vtkMultiBlockDataSet* vtkAMRDualClip::DoRequestData(
  vtkNonOverlappingAMR* hbdsInput, const char* arrayNameToProcess)
{
  vtkMultiBlockDataSet* mbdsOutput0 = vtkMultiBlockDataSet::New();
  mbdsOutput0->SetNumberOfBlocks(1);
  vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::New();
  mbdsOutput0->SetBlock(0, mpds);

  mpds->SetNumberOfPieces(0);

  if (this->Helper)
  {
    this->Helper->Delete();
  }

  this->Helper = vtkAMRDualGridHelper::New();
  this->Helper->SetEnableDegenerateCells(this->EnableDegenerateCells);
  if (this->EnableMultiProcessCommunication)
  {
    this->Helper->SetController(this->Controller);
  }
  else
  {
    this->Helper->SetController(NULL);
  }

  // @TODO: Check if this is the right thing to do.
  this->Helper->Initialize(hbdsInput);
  this->Helper->SetupData(hbdsInput, arrayNameToProcess);

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1 &&
    this->EnableDegenerateCells)
  {
    this->DistributeLevelMasks();
  }

  vtkUnstructuredGrid* mesh = vtkUnstructuredGrid::New();
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();
  mesh->SetPoints(this->Points);
  mpds->SetPiece(0, mesh);

  this->BlockIdCellArray = vtkIntArray::New();
  this->BlockIdCellArray->SetName("BlockIds");
  mesh->GetCellData()->AddArray(this->BlockIdCellArray);

  this->LevelMaskPointArray = vtkUnsignedCharArray::New();
  this->LevelMaskPointArray->SetName("LevelMask");
  mesh->GetPointData()->AddArray(this->LevelMaskPointArray);

  this->Mesh = mesh;
  this->InitializeCopyAttributes(hbdsInput, this->Mesh);

  // Loop through blocks
  int numLevels = hbdsInput->GetNumberOfLevels();
  int numBlocks;
  int blockId;

  // Add each block.
  for (int level = 0; level < numLevels; ++level)
  {
    numBlocks = this->Helper->GetNumberOfBlocksInLevel(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
      vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock(level, blockId);
      this->ProcessBlock(block, blockId, arrayNameToProcess);
    }
  }

  this->BlockIdCellArray->Delete();
  this->BlockIdCellArray = 0;
  this->LevelMaskPointArray->Delete();
  this->LevelMaskPointArray = 0;

  mesh->SetCells(VTK_TETRA, this->Cells);

  mesh->Delete();
  this->Mesh = 0;
  this->Points->Delete();
  this->Points = 0;
  this->Cells->Delete();
  this->Cells = 0;

  mpds->Delete();
  this->Helper->Delete();
  this->Helper = 0;

  return mbdsOutput0;
}

//----------------------------------------------------------------------------
// The only data specific stuff we need to do for the contour.
//----------------------------------------------------------------------------
template <class T>
void vtkDualGridContourCastCornerValues(T* ptr, vtkIdType offsets[8], double values[8])
{
  values[0] = (double)(ptr[offsets[0]]);
  values[1] = (double)(ptr[offsets[1]]);
  values[2] = (double)(ptr[offsets[2]]);
  values[3] = (double)(ptr[offsets[3]]);
  values[4] = (double)(ptr[offsets[4]]);
  values[5] = (double)(ptr[offsets[5]]);
  values[6] = (double)(ptr[offsets[6]]);
  values[7] = (double)(ptr[offsets[7]]);
}

//----------------------------------------------------------------------------
void vtkAMRDualClip::ShareBlockLocatorWithNeighbors(vtkAMRDualGridHelperBlock* block)
{
  vtkAMRDualGridHelperBlock* neighbor;
  // Blocks are processed low level to high so, we only need to share
  // the locator with blocks in the same level or higher.
  int numLevels = this->Helper->GetNumberOfLevels();
  int xMid, yMid, zMid;
  int xMin, xMax, yMin, yMax, zMin, zMax;

  for (int level = block->Level; level < numLevels; ++level)
  {
    // Neighborhood.
    int levelDiff = level - block->Level;
    xMid = block->GridIndex[0];
    xMin = (xMid << levelDiff) - 1;
    xMax = (xMid + 1) << levelDiff;
    yMid = block->GridIndex[1];
    yMin = (yMid << levelDiff) - 1;
    yMax = (yMid + 1) << levelDiff;
    zMid = block->GridIndex[2];
    zMin = (zMid << levelDiff) - 1;
    zMax = (zMid + 1) << levelDiff;

    // Lets just start with neighbors in the same level.
    for (int iz = zMin; iz <= zMax; ++iz)
    {
      for (int iy = yMin; iy <= yMax; ++iy)
      {
        for (int ix = xMin; ix <= xMax; ++ix)
        {
          if ((ix >> levelDiff) != xMid || (iy >> levelDiff) != yMid || (iz >> levelDiff) != zMid)
          {
            neighbor = this->Helper->GetBlock(level, ix, iy, iz);
            // The unused center flag is used as a flag to indicate
            if (neighbor && neighbor->Image && neighbor->RegionBits[1][1][1])
            {
              vtkAMRDualClipLocator* blockLocator = vtkAMRDualClipGetBlockLocator(block);
              blockLocator->ShareBlockLocatorWithNeighbor(block, neighbor);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// This is called before we start processing a block to make sure
// the locator is initialized in center and ghost regions.
void vtkAMRDualClip::InitializeLevelMask(vtkAMRDualGridHelperBlock* block)
{
  vtkImageData* image = block->Image;
  if (image == 0)
  { // Remote blocks are only to setup local block bit flags.
    return;
  }
  vtkDataArray* volumeFractionArray = image->GetCellData()->GetArray(this->Helper->GetArrayName());

  vtkAMRDualClipLocator* locator = vtkAMRDualClipGetBlockLocator(block);
  locator->ComputeLevelMask(volumeFractionArray, this->IsoValue, this->EnableInternalDecimation);

  vtkAMRDualGridHelperBlock* neighbor;
  vtkAMRDualClipLocator* neighborLocator;

  // We need to check for neighbors in lower or equal
  // to our level.  When blocks from different levels share a border
  // the high level block always owns the shared region.
  // This can be vastly simpler.  Since neighbor is always a lower (or equal)
  // level, each face/edge/corner has at most one neighbor.
  int xMid, yMid, zMid;
  int xMin, xMax, yMin, yMax, zMin, zMax;

  for (int level = 0; level <= block->Level; ++level)
  {
    // Neighborhood.
    int levelDiff = block->Level - level;
    xMid = block->GridIndex[0];
    xMin = (xMid >> levelDiff) - 1;
    xMax = (xMid + 1) >> levelDiff;
    yMid = block->GridIndex[1];
    yMin = (yMid >> levelDiff) - 1;
    yMax = (yMid + 1) >> levelDiff;
    zMid = block->GridIndex[2];
    zMin = (zMid >> levelDiff) - 1;
    zMax = (zMid + 1) >> levelDiff;

    for (int iz = zMin; iz <= zMax; ++iz)
    {
      for (int iy = yMin; iy <= yMax; ++iy)
      {
        for (int ix = xMin; ix <= xMax; ++ix)
        {
          if ((ix << levelDiff) != xMid || (iy << levelDiff) != yMid || (iz << levelDiff) != zMid)
          {
            // I could further prune and only copy to regions I own.
            neighbor = this->Helper->GetBlock(level, ix, iy, iz);
            // If the neighbor was already processed, then its level mask
            // was copied to this block already.
            if (neighbor && neighbor->RegionBits[1][1][1] != 0)
            {
              neighborLocator = vtkAMRDualClipGetBlockLocator(neighbor);
              image = neighbor->Image;
              if (image)
              {
                //                volumeFractionArray = this->GetInputArrayToProcess(0, image);
                volumeFractionArray = image->GetCellData()->GetArray(this->Helper->GetArrayName());
                neighborLocator->ComputeLevelMask(
                  volumeFractionArray, this->IsoValue, this->EnableInternalDecimation);
                locator->CopyNeighborLevelMask(block, neighbor);
              }
            }
          }
        }
      }
    }
  }

  // Take care of boundary faces which have not been set.
  // Just reflect values over face normal
  if (block->BoundaryBits & 1)
  {
    locator->CapLevelMaskFace(0, 0);
  }
  if (block->BoundaryBits & 2)
  {
    locator->CapLevelMaskFace(0, 1);
  }
  if (block->BoundaryBits & 4)
  {
    locator->CapLevelMaskFace(1, 0);
  }
  if (block->BoundaryBits & 8)
  {
    locator->CapLevelMaskFace(1, 1);
  }
  if (block->BoundaryBits & 16)
  {
    locator->CapLevelMaskFace(2, 0);
  }
  if (block->BoundaryBits & 32)
  {
    locator->CapLevelMaskFace(2, 1);
  }
}

//----------------------------------------------------------------------------
// This is called after we finished processing a block to share the locator
// level mask with neighbors before we delete the locator.
void vtkAMRDualClip::ShareLevelMask(vtkAMRDualGridHelperBlock* block)
{
  vtkAMRDualGridHelperBlock* neighbor;
  vtkAMRDualClipLocator* neighborLocator;
  int numLevels = this->Helper->GetNumberOfLevels();

  // We need to check for neighbors in higher or equal
  // to our level.  When blocks from different levels share a border
  // the high level block always owns the shared region.
  int xMid, yMid, zMid;
  int xMin, xMax, yMin, yMax, zMin, zMax;

  for (int level = block->Level; level < numLevels; ++level)
  {
    // Neighborhood.
    int levelDiff = level - block->Level;
    xMid = block->GridIndex[0];
    xMin = (xMid << levelDiff) - 1;
    xMax = (xMid + 1) << levelDiff;
    yMid = block->GridIndex[1];
    yMin = (yMid << levelDiff) - 1;
    yMax = (yMid + 1) << levelDiff;
    zMid = block->GridIndex[2];
    zMin = (zMid << levelDiff) - 1;
    zMax = (zMid + 1) << levelDiff;

    for (int iz = zMin; iz <= zMax; ++iz)
    {
      for (int iy = yMin; iy <= yMax; ++iy)
      {
        for (int ix = xMin; ix <= xMax; ++ix)
        {
          if ((ix >> levelDiff) != xMid || (iy >> levelDiff) != yMid || (iz >> levelDiff) != zMid)
          {
            // I could further prune and only copy to regions owned by neighbor.
            neighbor = this->Helper->GetBlock(level, ix, iy, iz);
            // If the neighbor was already processed, then its level mask
            // was copied to this block already.
            if (neighbor && neighbor->Image && neighbor->RegionBits[1][1][1] != 0)
            {
              neighborLocator = vtkAMRDualClipGetBlockLocator(neighbor);
              neighborLocator->CopyNeighborLevelMask(neighbor, block);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualClip::ProcessBlock(
  vtkAMRDualGridHelperBlock* block, int blockId, const char* arrayNameToProcess)
{
  vtkImageData* image = block->Image;
  if (image == 0)
  { // Remote blocks are only to setup local block bit flags.
    return;
  }

  // We are looking for only cell data arrays.
  vtkDataArray* volumeFractionArray = image->GetCellData()->GetArray(arrayNameToProcess);

  if (!volumeFractionArray)
  {
    return;
  }

  // void* volumeFractionPtr = volumeFractionArray->GetVoidPointer(0);
  double origin[3];
  double* spacing;
  int extent[6];

  // Get the origin and point extent of the dual grid (with ghost level).
  // This is the same as the cell extent of the original grid (with ghosts).
  image->GetExtent(extent);
  --extent[1];
  --extent[3];
  --extent[5];

  // Locator merges points in this block.
  // Input the dimensions of the dual cells with ghosts.
  if (this->EnableMergePoints)
  {
    this->InitializeLevelMask(block);
    this->BlockLocator = vtkAMRDualClipGetBlockLocator(block);
  }
  else
  { // Shared locator.
    if (this->BlockLocator == 0)
    {
      this->BlockLocator = new vtkAMRDualClipLocator;
    }
    this->BlockLocator->Initialize(
      extent[1] - extent[0], extent[3] - extent[2], extent[5] - extent[4]);
    // this->BlockLocator->CopyRegionLevelDifferences(block);
  }
  image->GetOrigin(origin);
  spacing = image->GetSpacing();
  // Dual cells are shifted half a pixel.
  origin[0] += 0.5 * spacing[0];
  origin[1] += 0.5 * spacing[1];
  origin[2] += 0.5 * spacing[2];

  // We deal with the various data types by copying the corner values
  // into a double array.  We have to cast anyway to compute the case.
  vtkIdType cornerOffsets[8];

  // The templated function needs the increments for pointers
  // cast to the correct datatype.
  int yInc = (extent[1] - extent[0] + 1);
  int zInc = yInc * (extent[3] - extent[2] + 1);

  // Loop over all the cells in the dual grid.
  int x, y, z;
  // These are needed to handle the cropped boundary cells.
  int xMax = extent[1] - 1;
  int yMax = extent[3] - 1;
  int zMax = extent[5] - 1;
  //-
  vtkIdType zOffset = 0;
  vtkIdType yOffset = 0;
  vtkIdType xOffset = 0;
  //-
  for (z = extent[4]; z < extent[5]; ++z)
  {
    int nz = 1;
    if (z == extent[4])
    {
      nz = 0;
    }
    else if (z == zMax)
    {
      nz = 2;
    }
    yOffset = zOffset;
    for (y = extent[2]; y < extent[3]; ++y)
    {
      int ny = 1;
      if (y == extent[2])
      {
        ny = 0;
      }
      else if (y == yMax)
      {
        ny = 2;
      }
      xOffset = yOffset;
      for (x = extent[0]; x < extent[1]; ++x)
      {
        int nx = 1;
        if (x == extent[0])
        {
          nx = 0;
        }
        else if (x == xMax)
        {
          nx = 2;
        }
        // Skip the cell if a neighbor is already processing it.
        if ((block->RegionBits[nx][ny][nz] & vtkAMRRegionBitOwner))
        {
          // Get the corner values as offsets
          cornerOffsets[0] = xOffset;
          cornerOffsets[1] = xOffset + 1;
          cornerOffsets[2] = xOffset + yInc;
          cornerOffsets[3] = xOffset + 1 + yInc;
          cornerOffsets[4] = xOffset + zInc;
          cornerOffsets[5] = xOffset + 1 + zInc;
          cornerOffsets[6] = xOffset + yInc + zInc;
          cornerOffsets[7] = xOffset + 1 + yInc + zInc;
          this->ProcessDualCell(block, blockId, x, y, z, cornerOffsets, volumeFractionArray);
        }
        xOffset += 1; // xInc
      }
      yOffset += yInc;
    }
    zOffset += zInc;
  }

  if (this->EnableMergePoints)
  {
    this->ShareLevelMask(block);
    // Copy point ids into neighbor locators.
    this->ShareBlockLocatorWithNeighbors(block);
    // We are done.  We no longer need the locator for this block.
    delete this->BlockLocator;
    this->BlockLocator = 0;
    block->UserData = 0;
    // Lets use this unused flag (owner of center region/block) to indicate
    // that the block is already processes.
    // This will keep neighbors from recreating the locator.
    // Another option would be to create the locator object for
    // all blocks but do not allocate until needed.  Then the existence of the locator
    // would tell whether the block was processed.
    block->RegionBits[1][1][1] = 0;
  }
}

//----------------------------------------------------------------------------
// Not implemented as optimally as we could.  It can be improved by making
// a fast path for internal cells (with no degeneracies).
void vtkAMRDualClip::ProcessDualCell(vtkAMRDualGridHelperBlock* block, int blockId, int x, int y,
  int z, vtkIdType cornerOffsets[8], vtkDataArray* volumeFractionArray)
{
  // compute the case index
  vtkImageData* image = block->Image;
  if (image == 0)
  { // Remote blocks are only to setup local block bit flags.
    return;
  }

  void* volumeFractionPtr = volumeFractionArray->GetVoidPointer(0);
  int dataType = volumeFractionArray->GetDataType();
  double cornerValues[8];
  switch (dataType)
  {
    vtkTemplateMacro(vtkDualGridContourCastCornerValues(
      (VTK_TT*)(volumeFractionPtr), cornerOffsets, cornerValues));
    default:
      vtkGenericWarningMacro("Execute: Unknown ScalarType");
  }
  // compute the case index
  int cubeIndex = 0;
  if (cornerValues[0] > this->IsoValue)
  {
    cubeIndex += 1;
  }
  if (cornerValues[1] > this->IsoValue)
  {
    cubeIndex += 2;
  }
  if (cornerValues[2] > this->IsoValue)
  {
    cubeIndex += 4;
  }
  if (cornerValues[3] > this->IsoValue)
  {
    cubeIndex += 8;
  }
  if (cornerValues[4] > this->IsoValue)
  {
    cubeIndex += 16;
  }
  if (cornerValues[5] > this->IsoValue)
  {
    cubeIndex += 32;
  }
  if (cornerValues[6] > this->IsoValue)
  {
    cubeIndex += 64;
  }
  if (cornerValues[7] > this->IsoValue)
  {
    cubeIndex += 128;
  }

  // I am trying to exit as quick as possible if there is
  // no surface to generate.  I could also check that the index
  // is not on boundary.4
  if (cubeIndex == 0)
  {
    return;
  }

  // Which boundaries does this cube/cell touch?
  unsigned char cubeBoundaryBits[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  // If this cell is degenerate, then remove triangles with 2 points.

  int nx, ny, nz; // Neighbor index [3][3][3];
  vtkIdType pointIds[6];
  double k, v0, v1;

  // Compute the spacing for this level and one lower level;
  const double* tmp = this->Helper->GetRootSpacing();
  double spacing[3];
  double lowerSpacing[3];
  for (int ii = 0; ii < 3; ++ii)
  {
    spacing[ii] = tmp[ii] / (double)(1 << block->Level);
    lowerSpacing[ii] = 2.0 * spacing[ii];
  }
  const double* origin = this->Helper->GetGlobalOrigin();

  // Use this range to  determine which dual point index is ghost.
  int ghostDualPointIndexRange[6];
  block->Image->GetExtent(ghostDualPointIndexRange);

  ghostDualPointIndexRange[0] += block->OriginIndex[0];
  ghostDualPointIndexRange[1] += block->OriginIndex[0] - 1;
  ghostDualPointIndexRange[2] += block->OriginIndex[1];
  ghostDualPointIndexRange[3] += block->OriginIndex[1] - 1;
  ghostDualPointIndexRange[4] += block->OriginIndex[2];
  ghostDualPointIndexRange[5] += block->OriginIndex[2] - 1;
  // Change to global index.
  int gx, gy, gz;
  gx = x + block->OriginIndex[0];
  gy = y + block->OriginIndex[1];
  gz = z + block->OriginIndex[2];

  double dx, dy, dz;       // Chop cells in half at boundary.
  double cornerPoints[32]; // 4 per point is easier to optimize than 3. (32 vs 24)
  // Loop over the corners.
  for (int c = 0; c < 8; ++c)
  {
    // The variables dx,dy,dz handle boundary cells.
    // They shift point by half a pixel on the boundary.
    dx = dy = dz = 0.5;
    // Place the point in one of the 26 ghost regions.
    int px, py, pz; // Corner global xyz index.
    // CornerIndex
    px = (c & 1) ? gx + 1 : gx;
    if (px == ghostDualPointIndexRange[0])
    {
      nx = 0;
      if ((block->BoundaryBits & 1))
      {
        dx = 1.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 1;
      }
    }
    else if (px == ghostDualPointIndexRange[1])
    {
      nx = 2;
      if ((block->BoundaryBits & 2))
      {
        dx = 0.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 2;
      }
    }
    else
    {
      nx = 1;
    }
    py = (c & 2) ? gy + 1 : gy;
    if (py == ghostDualPointIndexRange[2])
    {
      ny = 0;
      if ((block->BoundaryBits & 4))
      {
        dy = 1.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 4;
      }
    }
    else if (py == ghostDualPointIndexRange[3])
    {
      ny = 2;
      if ((block->BoundaryBits & 8))
      {
        dy = 0.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 8;
      }
    }
    else
    {
      ny = 1;
    }
    pz = (c & 4) ? gz + 1 : gz;
    if (pz == ghostDualPointIndexRange[4])
    {
      nz = 0;
      if ((block->BoundaryBits & 16))
      {
        dz = 1.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 16;
      }
    }
    else if (pz == ghostDualPointIndexRange[5])
    {
      nz = 2;
      if ((block->BoundaryBits & 32))
      {
        dz = 0.0;
        cubeBoundaryBits[c] = cubeBoundaryBits[c] | 32;
      }
    }
    else
    {
      nz = 1;
    }

    if (block->RegionBits[nx][ny][nz] & vtkAMRRegionBitsDegenerateMask)
    { // point lies in lower level neighbor.
      int levelDiff = block->RegionBits[nx][ny][nz] & vtkAMRRegionBitsDegenerateMask;
      px = px >> levelDiff;
      py = py >> levelDiff;
      pz = pz >> levelDiff;
      // Shift half a pixel to get center of cell (dual point).
      if (levelDiff == 1)
      { // This is the most common case; avoid extra multiplications.
        cornerPoints[c << 2] = origin[0] + lowerSpacing[0] * ((double)(px) + dx);
        cornerPoints[(c << 2) | 1] = origin[1] + lowerSpacing[1] * ((double)(py) + dy);
        cornerPoints[(c << 2) | 2] = origin[2] + lowerSpacing[2] * ((double)(pz) + dz);
      }
      else
      { // This could be the only degenerate path with a little extra cost.
        cornerPoints[c << 2] =
          origin[0] + spacing[0] * (double)(1 << levelDiff) * ((double)(px) + dx);
        cornerPoints[(c << 2) | 1] =
          origin[1] + spacing[1] * (double)(1 << levelDiff) * ((double)(py) + dy);
        cornerPoints[(c << 2) | 2] =
          origin[2] + spacing[2] * (double)(1 << levelDiff) * ((double)(pz) + dz);
      }
    }
    else
    {
      // How do I chop the cells in half on the boundaries?
      // Move the origin and change spacing.
      cornerPoints[c << 2] = origin[0] + spacing[0] * ((double)(px) + dx);
      cornerPoints[(c << 2) | 1] = origin[1] + spacing[1] * ((double)(py) + dy);
      cornerPoints[(c << 2) | 2] = origin[2] + spacing[2] * ((double)(pz) + dz);
    }
  }
  // We have the points, now contour the cell.
  // Get edges.
  double pt[3];
  int* tetra = vtkAMRDualClipTetraTable[cubeIndex];
  vtkIdType* ptIdPtr;

  // loop over tetras
  while (*tetra > -1)
  {
    // Create brute force locator for a block, and resuse it.
    // Only permanently keep locator for edges shared between two blocks.
    for (int ii = 0; ii < 4; ++ii, ++tetra) // insert tetra
    {
      unsigned char levelMaskValue = 0;
      int casePtId = *tetra;
      // convert from VTK corner ids to bit (x,y,z) corner ids.
      if (casePtId < 8)
      { // Corner (internal point)
        ptIdPtr = this->BlockLocator->GetCornerPointer(x, y, z, casePtId, block->OriginIndex);
        levelMaskValue = this->BlockLocator->GetLevelMaskValue(
          x + ((casePtId & 1) ? 1 : 0), y + ((casePtId & 2) ? 1 : 0), z + ((casePtId & 4) ? 1 : 0));
        if (levelMaskValue == 0)
        { // bug !!!!! trying to figure out what is going on.
          levelMaskValue = 1;
        }
        if (*ptIdPtr == -1)
        {
          // I am not going to tamper with the computation of points on the surface
          // because it is working well.
          // Compute points for interior dual cells based on the
          // level mask degeneracy in the locator.
          // We could skip computing the "cornerpoints" for case 255 (all internal).
          // It is a pain to handle boundaries here with duplicate code but oh well.
          // Hey! The results of the last pass were saved in cube boundary bits.
          dx = dy = dz = 0.5;
          if (cubeBoundaryBits[casePtId] & 1)
          {
            dx = 1.0;
          }
          if (cubeBoundaryBits[casePtId] & 2)
          {
            dx = 0.0;
          }
          if (cubeBoundaryBits[casePtId] & 4)
          {
            dy = 1.0;
          }
          if (cubeBoundaryBits[casePtId] & 8)
          {
            dy = 0.0;
          }
          if (cubeBoundaryBits[casePtId] & 16)
          {
            dz = 1.0;
          }
          if (cubeBoundaryBits[casePtId] & 32)
          {
            dz = 0.0;
          }

          int levelDiff = levelMaskValue - 1; // Had to shift mask so 0 would be a special value.
          int px = (casePtId & 1) ? gx + 1 : gx;
          int py = (casePtId & 2) ? gy + 1 : gy;
          int pz = (casePtId & 4) ? gz + 1 : gz;
          px = px >> levelDiff;
          py = py >> levelDiff;
          pz = pz >> levelDiff;
          // Shifting half an index to get to the middle of the cell for the dual point.
          pt[0] = origin[0] + spacing[0] * (double)(1 << levelDiff) * ((double)(px) + dx);
          pt[1] = origin[1] + spacing[1] * (double)(1 << levelDiff) * ((double)(py) + dy);
          pt[2] = origin[2] + spacing[2] * (double)(1 << levelDiff) * ((double)(pz) + dz);
          *ptIdPtr = this->Points->InsertNextPoint(pt);
          if (pt[1] > 100000.0)
          {
            cerr << "bug\n";
          }

          // For internal points we do not need to interpolate attributes.
          // We just need to copy from a cell.  I do not know what to do for decimated degeneracy.
          // I suppose the correct solution would be to average all high-res values in the
          // degenerate
          // lower level cell bounds, but that would be too dificult.  Just pick one.
          // Averaging could be a pre processing step but we would have to modify input attributes
          // .......
          vtkIdType offset = cornerOffsets[casePtId];
          this->Mesh->GetPointData()->CopyData(block->Image->GetCellData(), offset, *ptIdPtr);

          this->LevelMaskPointArray->InsertNextValue(levelMaskValue);
        }
      }
      else
      { // Edge (clipped cell, point on iso surface)
        ptIdPtr = this->BlockLocator->GetEdgePointer(x, y, z, casePtId - 8);
        if (*ptIdPtr == -1)
        {
          int edge = casePtId - 8;
          // Compute the interpolation factor.
          v0 = cornerValues[vtkAMRDualIsoEdgeToPointsTable[edge][0]];
          v1 = cornerValues[vtkAMRDualIsoEdgeToPointsTable[edge][1]];
          k = (this->IsoValue - v0) / (v1 - v0);
          // Add the point to the output and get the index of the point.
          int pt1Idx = (vtkAMRDualIsoEdgeToPointsTable[edge][0] << 2);
          int pt2Idx = (vtkAMRDualIsoEdgeToPointsTable[edge][1] << 2);
          // I wonder if this is any faster than incrementing a pointer.
          pt[0] = cornerPoints[pt1Idx] + k * (cornerPoints[pt2Idx] - cornerPoints[pt1Idx]);
          pt[1] =
            cornerPoints[pt1Idx | 1] + k * (cornerPoints[pt2Idx | 1] - cornerPoints[pt1Idx | 1]);
          pt[2] =
            cornerPoints[pt1Idx | 2] + k * (cornerPoints[pt2Idx | 2] - cornerPoints[pt1Idx | 2]);
          *ptIdPtr = this->Points->InsertNextPoint(pt);
          if (pt[1] > 100000.0)
          {
            cerr << "bug\n";
          }

          // Interpolate attributes
          // Find the offsets of the two attributes to interpolate
          vtkIdType offset0 = cornerOffsets[pt1Idx >> 2];
          vtkIdType offset1 = cornerOffsets[pt2Idx >> 2];
          this->Mesh->GetPointData()->InterpolateEdge(
            block->Image->GetCellData(), *ptIdPtr, offset0, offset1, k);

          this->LevelMaskPointArray->InsertNextValue(levelMaskValue);
        }
      }
      pointIds[ii] = *ptIdPtr;
    }
    if (pointIds[0] != pointIds[1] && pointIds[0] != pointIds[2] && pointIds[0] != pointIds[3] &&
      pointIds[1] != pointIds[2] && pointIds[1] != pointIds[3] && pointIds[2] != pointIds[3])
    {
      this->Cells->InsertNextCell(4, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
    }
  }
}

//----------------------------------------------------------------------------
// In order to synchronize level mask between processes we need to precompute
// them.  It is a shame that we cannot generate the masks on demand.
// This method is meant to be called right after initialization.
// Level masks only move regions from lower levels to higher levels.
void vtkAMRDualClip::DistributeLevelMasks()
{
  vtkAMRDualGridHelperBlock* block;
  vtkAMRDualGridHelperBlock* neighborBlock;

  if (this->Controller == 0)
  {
    return;
  }
  this->Helper->ClearRegionRemoteCopyQueue();

  // Make a map of interprocess commnication.
  // Each region has a process, level, grid index, and extent.
  int myProcessId = this->Controller->GetLocalProcessId();

  // Loop through blocks
  int numLevels = this->Helper->GetNumberOfLevels();
  int numBlocks;
  int blockId;

  // Process each block.
  for (int level = 0; level < numLevels; ++level)
  {
    numBlocks = this->Helper->GetNumberOfBlocksInLevel(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
      block = this->Helper->GetBlock(level, blockId);

      // Any blocks sending to this block from lower levels?
      // Lets look by region.
      for (int rz = -1; rz < 2; ++rz)
      {
        for (int ry = -1; ry < 2; ++ry)
        {
          for (int rx = -1; rx < 2; ++rx)
          {
            if (block->RegionBits[rx + 1][ry + 1][rz + 1] & vtkAMRRegionBitOwner)
            {
              for (int lowerLevel = 0; lowerLevel <= level; ++lowerLevel)
              {
                // Convert the grid index into the lower level coordinate system.
                int xGrid = (block->GridIndex[0] + rx) >> (block->Level - lowerLevel);
                int yGrid = (block->GridIndex[1] + ry) >> (block->Level - lowerLevel);
                int zGrid = (block->GridIndex[2] + rz) >> (block->Level - lowerLevel);
                neighborBlock = this->Helper->GetBlock(lowerLevel, xGrid, yGrid, zGrid);
                // We can ignore pairs in the same process.
                if (neighborBlock && neighborBlock->ProcessId != block->ProcessId)
                {
                  // We can ignore pairs if both are in other processes.
                  if (block->ProcessId == myProcessId || neighborBlock->ProcessId == myProcessId)
                  {
                    const char* arrayName = this->Helper->GetArrayName();
                    vtkDataArray* scalars;
                    vtkDataArray* neighborLevelMaskArray = 0;
                    vtkDataArray* blockLevelMaskArray = 0;
                    if (block->Image)
                    {
                      scalars = block->Image->GetCellData()->GetArray(arrayName);
                      vtkAMRDualClipLocator* blockLocator = vtkAMRDualClipGetBlockLocator(block);
                      blockLocator->ComputeLevelMask(
                        scalars, this->IsoValue, this->EnableInternalDecimation);
                      blockLevelMaskArray = blockLocator->GetLevelMaskArray();
                    }
                    if (neighborBlock->Image)
                    {
                      scalars = neighborBlock->Image->GetCellData()->GetArray(arrayName);
                      vtkAMRDualClipLocator* neighborLocator =
                        vtkAMRDualClipGetBlockLocator(neighborBlock);
                      neighborLocator->ComputeLevelMask(
                        scalars, this->IsoValue, this->EnableInternalDecimation);
                      neighborLevelMaskArray = neighborLocator->GetLevelMaskArray();
                    }

                    this->Helper->QueueRegionRemoteCopy(rx, ry, rz, neighborBlock,
                      neighborLevelMaskArray, block, blockLevelMaskArray);
                  } // if pair in queue
                }   // if one block is in our processes
              }     // Loop over source levels.
            }       // if the receiving block owns the region.
          }         // loop over region x index
        }           // loop over region y index
      }             // loop over region z index
    }               // loop over receiving blocks in level
  }                 // loop over all levels

  this->Helper->ProcessRegionRemoteCopyQueue(true);
}

//----------------------------------------------------------------------------
void vtkAMRDualClip::InitializeCopyAttributes(vtkNonOverlappingAMR* hbdsInput, vtkDataSet* mesh)
{
  // Most of this is just getting a block with cell attributes so we can
  // call CopyAllocate.
  vtkCompositeDataIterator* iter = hbdsInput->NewIterator();
  iter->InitTraversal();
  if (iter->IsDoneWithTraversal())
  { // Empty input
    iter->Delete();
    return;
  }
  vtkDataObject* dataObject = iter->GetCurrentDataObject();
  vtkUniformGrid* uGrid = vtkUniformGrid::SafeDownCast(dataObject);
  if (uGrid == 0)
  {
    vtkErrorMacro("Expecting a uniform grid.");
  }
  mesh->GetPointData()->CopyAllocate(uGrid->GetCellData());
  iter->Delete();
}
