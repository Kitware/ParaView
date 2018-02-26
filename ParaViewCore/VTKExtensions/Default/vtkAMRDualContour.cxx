/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDualContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRDualContour.h"
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
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include <ctime>
#include <math.h>

vtkStandardNewMacro(vtkAMRDualContour);

vtkCxxSetObjectMacro(vtkAMRDualContour, Controller, vtkMultiProcessController);

static int vtkAMRDualIsoEdgeToPointsTable[12][2] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 },
  { 4, 5 }, { 5, 7 }, { 6, 7 }, { 4, 6 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
static int vtkAMRDualIsoEdgeToVTKPointsTable[12][2] = { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 },
  { 4, 5 }, { 5, 6 }, { 7, 6 }, { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } };

// It is working but we have some missing features.
// 1: Make a Clip Filter
// 2: Merge points.
// 3: Change degenerate quads to tris or remove.
// 4: Copy Attributes from input to output.

//============================================================================
// Used separately for each block.  This is the typical 3 edge per voxel
// lookup.  We do need to worry about degeneracy because corners can merge
// and edges can merge when the degenerate cell is a wedge.
class vtkAMRDualContourEdgeLocator
{
public:
  vtkAMRDualContourEdgeLocator();
  ~vtkAMRDualContourEdgeLocator();

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
  vtkIdType* GetCornerPointer(int xCell, int yCell, int zCell, int cornerIdx);

  // Description:
  // To handle degenerate cells, indicate the level difference between the block
  // and region neighbor.
  void CopyRegionLevelDifferences(vtkAMRDualGridHelperBlock* block);

  // Description:
  // Deprecciated
  // Used to share point ids between block locators.
  void SharePointIdsWithNeighbor(
    vtkAMRDualContourEdgeLocator* neighborLocator, int rx, int ry, int rz);

  void ShareBlockLocatorWithNeighbor(
    vtkAMRDualGridHelperBlock* block, vtkAMRDualGridHelperBlock* neighbor);

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

  int RegionLevelDifference[3][3][3];
};
//----------------------------------------------------------------------------
void vtkAMRDualContourEdgeLocator::CopyRegionLevelDifferences(vtkAMRDualGridHelperBlock* block)
{
  int x, y, z;
  for (z = 0; z < 3; ++z)
  {
    for (y = 0; y < 3; ++y)
    {
      for (x = 0; x < 3; ++x)
      {
        this->RegionLevelDifference[x][y][z] =
          block->RegionBits[x][y][z] & vtkAMRRegionBitsDegenerateMask;
      }
    }
  }
}
//----------------------------------------------------------------------------
vtkAMRDualContourEdgeLocator::vtkAMRDualContourEdgeLocator()
{
  this->DualCellDimensions[0] = 0;
  this->DualCellDimensions[1] = 0;
  this->DualCellDimensions[2] = 0;
  this->YIncrement = this->ZIncrement = 0;
  this->ArrayLength = 0;
  this->XEdges = this->YEdges = this->ZEdges = 0;
  this->Corners = 0;
}
//----------------------------------------------------------------------------
vtkAMRDualContourEdgeLocator::~vtkAMRDualContourEdgeLocator()
{
  this->Initialize(0, 0, 0);
}
//----------------------------------------------------------------------------
void vtkAMRDualContourEdgeLocator::Initialize(int xDualCellDim, int yDualCellDim, int zDualCellDim)
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

  int x, y, z;
  for (z = 0; z < 3; ++z)
  {
    for (y = 0; y < 3; ++y)
    {
      for (x = 0; x < 3; ++x)
      {
        this->RegionLevelDifference[x][y][z] = 0;
      }
    }
  }
}

//----------------------------------------------------------------------------
// No bounds checking.
// I am going to move points that are very close to a corner to a corner
// I assume this will improve the mesh.
vtkIdType* vtkAMRDualContourEdgeLocator::GetEdgePointer(
  int xCell, int yCell, int zCell, int edgeIdx)
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
  // template index is also the first point index.
  // Find the second point index.
  int xp1 = xp0;
  int yp1 = yp0;
  int zp1 = zp0;
  if (axis == 1)
  {
    ++xp1;
  }
  else if (axis == 2)
  {
    ++yp1;
  }
  else if (axis == 4)
  {
    ++zp1;
  }

  // Now we can adjust the cell index based on degeneracy.
  // This is tricky with edges.  Two points in two regions.
  // We can ignore any edge that collapses to a point.
  // We only have to consider to edges becoming one.
  int diff0, diff1;
  int rx0, ry0, rz0;
  rx0 = ry0 = rz0 = 1;
  if (xp0 == 0)
  {
    rx0 = 0;
  }
  if (xp0 == this->DualCellDimensions[0])
  {
    rx0 = 2;
  }
  if (yp0 == 0)
  {
    ry0 = 0;
  }
  if (yp0 == this->DualCellDimensions[1])
  {
    ry0 = 2;
  }
  if (zp0 == 0)
  {
    rz0 = 0;
  }
  if (zp0 == this->DualCellDimensions[2])
  {
    rz0 = 2;
  }
  diff0 = this->RegionLevelDifference[rx0][ry0][rz0];
  int rx1, ry1, rz1;
  rx1 = ry1 = rz1 = 1;
  if (xp1 == 0)
  {
    rx1 = 0;
  }
  if (xp1 == this->DualCellDimensions[0])
  {
    rx1 = 2;
  }
  if (yp1 == 0)
  {
    ry1 = 0;
  }
  if (yp1 == this->DualCellDimensions[1])
  {
    ry1 = 2;
  }
  if (zp1 == 0)
  {
    rz1 = 0;
  }
  if (zp1 == this->DualCellDimensions[2])
  {
    rz1 = 2;
  }
  diff1 = this->RegionLevelDifference[rx1][ry1][rz1];
  // Take the minimum diff because one unique point makes a unique edge.
  if (diff1 < diff0)
  {
    diff0 = diff1;
  }
  // Is does not matter what we do with edges that collase to a point
  // because the isosurface will never split the two.
  if (diff0)
  {
    if (rx0 == 1 && xp0 > 0)
    {
      xp0 = (((xp0 - 1) >> diff0) << diff0) + 1;
    }
    if (ry0 == 1 && yp0 > 0)
    {
      yp0 = (((yp0 - 1) >> diff0) << diff0) + 1;
    }
    if (rz0 == 1 && zp0 > 0)
    {
      zp0 = (((zp0 - 1) >> diff0) << diff0) + 1;
    }
    // I do not see how these are needed but ...
    if (rx1 == 1 && xp1 > 0)
    {
      xp1 = (((xp1 - 1) >> diff0) << diff0) + 1;
    }
    if (ry1 == 1 && yp1 > 0)
    {
      yp1 = (((yp1 - 1) >> diff0) << diff0) + 1;
    }
    if (rz1 == 1 && zp1 > 0)
    {
      zp1 = (((zp1 - 1) >> diff0) << diff0) + 1;
    }
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
vtkIdType* vtkAMRDualContourEdgeLocator::GetCornerPointer(
  int xCell, int yCell, int zCell, int cornerIdx)
{
  int diff;
  xCell += cornerIdx & 1;
  yCell += (cornerIdx & 2) >> 1;
  zCell += (cornerIdx & 4) >> 2;

  // For degenerate regions we do not need to worry about corners.
  // Too many cases (18)  generalize it.
  int rx, ry, rz;
  rx = ry = rz = 1;
  if (xCell == 0)
  {
    rx = 0;
  }
  if (xCell == this->DualCellDimensions[0])
  {
    rx = 2;
  }
  if (yCell == 0)
  {
    ry = 0;
  }
  if (yCell == this->DualCellDimensions[1])
  {
    ry = 2;
  }
  if (zCell == 0)
  {
    rz = 0;
  }
  if (zCell == this->DualCellDimensions[2])
  {
    rz = 2;
  }

  if ((diff = this->RegionLevelDifference[rx][ry][rz]))
  {
    if (rx == 1 && xCell > 0)
    {
      xCell = (((xCell - 1) >> diff) << diff) + 1;
    }
    if (ry == 1 && yCell > 0)
    {
      yCell = (((yCell - 1) >> diff) << diff) + 1;
    }
    if (rz == 1 && zCell > 0)
    {
      zCell = (((zCell - 1) >> diff) << diff) + 1;
    }
  }

  return this->Corners + (xCell + (yCell * this->YIncrement) + (zCell * this->ZIncrement));
}

//----------------------------------------------------------------------------
vtkAMRDualContourEdgeLocator* vtkAMRDualContourGetBlockLocator(vtkAMRDualGridHelperBlock* block)
{
  if (block->UserData == 0)
  {
    vtkImageData* image = block->Image;
    if (image == 0)
    { // Remote blocks are only to setup local block bit flags.
      return 0;
    }
    int extent[6];
    // This is the same as the cell extent of the original grid (with ghosts).
    image->GetExtent(extent);
    --extent[1];
    --extent[3];
    --extent[5];

    vtkAMRDualContourEdgeLocator* locator = new vtkAMRDualContourEdgeLocator;
    block->UserData = (void*)(locator); // Block owns it now.
    locator->Initialize(extent[1] - extent[0], extent[3] - extent[2], extent[5] - extent[4]);
    locator->CopyRegionLevelDifferences(block);
    return locator;
  }
  return (vtkAMRDualContourEdgeLocator*)(block->UserData);
}

//----------------------------------------------------------------------------
// This version works with higher level neighbor blocks.
void vtkAMRDualContourEdgeLocator::ShareBlockLocatorWithNeighbor(
  vtkAMRDualGridHelperBlock* block, vtkAMRDualGridHelperBlock* neighbor)
{
  vtkAMRDualContourEdgeLocator* blockLocator = vtkAMRDualContourGetBlockLocator(block);
  vtkAMRDualContourEdgeLocator* neighborLocator = vtkAMRDualContourGetBlockLocator(neighbor);

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
vtkAMRDualContour::vtkAMRDualContour()
{
  this->IsoValue = 100.0;
  this->SkipGhostCopy = 0;

  this->EnableDegenerateCells = 1;
  this->EnableCapping = 1;
  this->EnableMultiProcessCommunication = 1;
  this->EnableMergePoints = 1;
  this->TriangulateCap = 1;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  // Pipeline
  this->SetNumberOfOutputPorts(1);

  this->TemperatureArray = 0;
  this->BlockIdCellArray = 0;
  this->Helper = 0;

  this->BlockLocator = 0;
}

//----------------------------------------------------------------------------
vtkAMRDualContour::~vtkAMRDualContour()
{
  if (this->BlockLocator)
  {
    delete this->BlockLocator;
    this->BlockLocator = 0;
  }
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IsoValue: " << this->IsoValue << endl;
  os << indent << "EnableCapping: " << this->EnableCapping << endl;
  os << indent << "EnableDegenerateCells: " << this->EnableDegenerateCells << endl;
  os << indent << "EnableMultiProcessCommunication: " << this->EnableMultiProcessCommunication
     << endl;
  os << indent << "EnableMergePoints: " << this->EnableMergePoints << endl;
  os << indent << "TriangulateCap: " << this->TriangulateCap << endl;
  os << indent << "SkipGhostCopy: " << this->SkipGhostCopy << endl;
}

//----------------------------------------------------------------------------
int vtkAMRDualContour::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRDualContour::FillOutputPortInformation(int port, vtkInformation* info)
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
int vtkAMRDualContour::RequestData(vtkInformation* vtkNotUsed(request),
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

  mbdsOutput0->SetNumberOfBlocks(1);
  vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::New();
  mbdsOutput0->SetBlock(0, mpds);

  mpds->SetNumberOfPieces(0);

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

  this->InitializeRequest(hbdsInput);
  vtkMultiBlockDataSet* out = this->DoRequestData(hbdsInput, arrayNameToProcess);
  this->FinalizeRequest();

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

void vtkAMRDualContour::InitializeRequest(vtkNonOverlappingAMR* hbdsInput)
{
  if (this->Helper)
  {
    this->Helper->Delete();
  }

  this->Helper = vtkAMRDualGridHelper::New();
  this->Helper->SetEnableDegenerateCells(this->EnableDegenerateCells);
  this->Helper->SetSkipGhostCopy(this->SkipGhostCopy);
  if (this->EnableMultiProcessCommunication)
  {
    this->Helper->SetController(this->Controller);
  }
  else
  {
    this->Helper->SetController(NULL);
  }
  this->Helper->Initialize(hbdsInput);
}

void vtkAMRDualContour::FinalizeRequest()
{
  this->Helper->Delete();
  this->Helper = 0;
}

vtkMultiBlockDataSet* vtkAMRDualContour::DoRequestData(
  vtkNonOverlappingAMR* hbdsInput, const char* arrayNameToProcess)
{
  this->Helper->SetupData(hbdsInput, arrayNameToProcess);

  vtkMultiBlockDataSet* mbdsOutput0 = vtkMultiBlockDataSet::New();
  mbdsOutput0->SetNumberOfBlocks(1);
  vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::New();
  mbdsOutput0->SetBlock(0, mpds);

  mpds->SetNumberOfPieces(0);

  this->Mesh = vtkPolyData::New();
  this->Points = vtkPoints::New();
  this->Faces = vtkCellArray::New();
  this->Mesh->SetPoints(this->Points);
  this->Mesh->SetPolys(this->Faces);
  mpds->SetPiece(0, this->Mesh);

  this->InitializeCopyAttributes(hbdsInput, this->Mesh);

  // For debugging.
  this->BlockIdCellArray = vtkIntArray::New();
  this->BlockIdCellArray->SetName("BlockIds");
  this->Mesh->GetCellData()->AddArray(this->BlockIdCellArray);

  // Loop through blocks
  int numLevels = hbdsInput->GetNumberOfLevels();

  // Add each block.
  for (int level = 0; level < numLevels; ++level)
  {
    int numBlocks = this->Helper->GetNumberOfBlocksInLevel(level);
    for (int blockId = 0; blockId < numBlocks; ++blockId)
    {
      vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock(level, blockId);
      this->ProcessBlock(block, blockId, arrayNameToProcess);
    }
  }

  this->FinalizeCopyAttributes(this->Mesh);
  this->BlockIdCellArray->Delete();
  this->BlockIdCellArray = 0;

  this->Mesh->Delete();
  this->Mesh = 0;
  this->Points->Delete();
  this->Points = 0;
  this->Faces->Delete();
  this->Faces = 0;

  mpds->Delete();

  return mbdsOutput0;
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::ShareBlockLocatorWithNeighbors(vtkAMRDualGridHelperBlock* block)
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
              vtkAMRDualContourEdgeLocator* blockLocator = vtkAMRDualContourGetBlockLocator(block);
              blockLocator->ShareBlockLocatorWithNeighbor(block, neighbor);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::ProcessBlock(
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
    this->BlockLocator = vtkAMRDualContourGetBlockLocator(block);
  }
  else
  { // Shared locator.
    if (this->BlockLocator == 0)
    {
      this->BlockLocator = new vtkAMRDualContourEdgeLocator;
    }
    this->BlockLocator->Initialize(
      extent[1] - extent[0], extent[3] - extent[2], extent[5] - extent[4]);
    this->BlockLocator->CopyRegionLevelDifferences(block);
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
  // Use void pointers to march through the volume before we cast.
  // int dataType = volumeFractionArray->GetDataType();
  // int xVoidInc = volumeFractionArray->GetDataTypeSize();
  // int yVoidInc = xVoidInc * yInc;
  // int zVoidInc = xVoidInc * zInc;

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
          cornerOffsets[2] = xOffset + 1 + yInc;
          cornerOffsets[3] = xOffset + yInc;
          cornerOffsets[4] = xOffset + zInc;
          cornerOffsets[5] = xOffset + 1 + zInc;
          cornerOffsets[6] = xOffset + 1 + yInc + zInc;
          cornerOffsets[7] = xOffset + yInc + zInc;
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

// Generic table for clipping a square.
// We can have two polygons.
// Polygons are separated by -1, and terminated by -2.
// 0-3 are the corners of the square (00,10,11,01).
// 4-7 are for points on the edges (4:(00-10),5:(10-11),6:(11-01),7:(01-00)
static int vtkAMRDualIsoCappingTable[16][8] = { { -2, 0, 0, 0, 0, 0, 0, 0 }, //(0000)
  { 0, 4, 7, -2, 0, 0, 0, 0 },                                               //(1000)
  { 1, 5, 4, -2, 0, 0, 0, 0 },                                               //(0100)
  { 0, 1, 5, 7, -2, 0, 0, 0 },                                               //(1100)
  { 2, 6, 5, -2, 0, 0, 0, 0 },                                               //(0010)
  { 0, 4, 7, -1, 2, 6, 5, -2 },                                              //(1010)
  { 1, 2, 6, 4, -2, 0, 0, 0 },                                               //(0110)
  { 0, 1, 2, 6, 7, -2, 0, 0 },                                               //(1110)
  { 3, 7, 6, -2, 0, 0, 0, 0 },                                               //(0001)
  { 3, 0, 4, 6, -2, 0, 0, 0 },                                               //(1001)
  { 1, 5, 4, -1, 3, 7, 6, -2 },                                              //(0101)
  { 3, 0, 1, 5, 6, -2, 0, 0 },                                               //(1101)
  { 2, 3, 7, 5, -2, 0, 0, 0 },                                               //(0011)
  { 2, 3, 0, 4, 5, -2, 0, 0 },                                               //(1011)
  { 1, 2, 3, 7, 4, -2, 0, 0 },                                               //(0111)
  { 0, 1, 2, 3, -2, 0, 0, 0 } };                                             //(1111)

// These tables map the corners and edges from the above table
// into corners and edges for the face of a cube.
// First for map 0-3 into corners 0-7 000,100,010,110,001,101,011,111
// Edges 4-7 get mapped to standard cube edge index.
// 0:(000-100),1:(100-110),2:(110-010),3:(010-000),
static int vtkAMRDualIsoNXCapEdgeMap[8] = { 0, 2, 6, 4, 3, 10, 7, 8 };
static int vtkAMRDualIsoPXCapEdgeMap[8] = { 1, 5, 7, 3, 9, 5, 11, 1 };

static int vtkAMRDualIsoNYCapEdgeMap[8] = { 0, 4, 5, 1, 8, 4, 9, 0 };
static int vtkAMRDualIsoPYCapEdgeMap[8] = { 2, 3, 7, 6, 2, 11, 6, 10 };

static int vtkAMRDualIsoNZCapEdgeMap[8] = { 0, 1, 3, 2, 0, 1, 2, 3 };
static int vtkAMRDualIsoPZCapEdgeMap[8] = { 6, 7, 5, 4, 6, 5, 4, 7 };

// WHat a pain using two different vertex orders.
static int vtkAMRDualLegacyIdToBitIdMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };

//----------------------------------------------------------------------------
// Not implemented as optimally as we could.  It can be improved by making
// a fast path for internal cells (with no degeneracies).
// Corner offsets are absolute (relative to origin / 0).
void vtkAMRDualContour::ProcessDualCell(vtkAMRDualGridHelperBlock* block, int blockId, int x, int y,
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

  unsigned char cubeCase = 0;
  if (cornerValues[0] > this->IsoValue)
  {
    cubeCase += 1;
  }
  if (cornerValues[1] > this->IsoValue)
  {
    cubeCase += 2;
  }
  if (cornerValues[2] > this->IsoValue)
  {
    cubeCase += 4;
  }
  if (cornerValues[3] > this->IsoValue)
  {
    cubeCase += 8;
  }
  if (cornerValues[4] > this->IsoValue)
  {
    cubeCase += 16;
  }
  if (cornerValues[5] > this->IsoValue)
  {
    cubeCase += 32;
  }
  if (cornerValues[6] > this->IsoValue)
  {
    cubeCase += 64;
  }
  if (cornerValues[7] > this->IsoValue)
  {
    cubeCase += 128;
  }

  // I am trying to exit as quick as possible if there is
  // no surface to generate.  I could also check that the index
  // is not on boundary.
  if (cubeCase == 0 || (cubeCase == 255 && block->BoundaryBits == 0))
  {
    return;
  }

  // Which boundaries does this cube/cell touch?
  unsigned char cubeBoundaryBits = 0;

  int nx, ny, nz; // Neighbor index [3][3][3];
  vtkIdType pointIds[6];
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST* edge;
  double k, v0, v1;
  triCases = vtkMarchingCubesTriangleCases::GetCases();

  // Compute the spacing for this level and one lower level;
  const double* tmp = this->Helper->GetRootSpacing();
  double spacing[3];
  double lowerSpacing[3];
  for (int ii = 0; ii < 3; ++ii)
  {
    spacing[ii] = tmp[ii] / (double)(1 << block->Level);
    lowerSpacing[ii] = 2.0 * spacing[ii];
  }
  tmp = this->Helper->GetGlobalOrigin();

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
  double cornerPoints[32]; // 4 is easier to optimize than 3.
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
        cubeBoundaryBits = cubeBoundaryBits | 1;
      }
    }
    else if (px == ghostDualPointIndexRange[1])
    {
      nx = 2;
      if ((block->BoundaryBits & 2))
      {
        dx = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 2;
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
        cubeBoundaryBits = cubeBoundaryBits | 4;
      }
    }
    else if (py == ghostDualPointIndexRange[3])
    {
      ny = 2;
      if ((block->BoundaryBits & 8))
      {
        dy = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 8;
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
        cubeBoundaryBits = cubeBoundaryBits | 16;
      }
    }
    else if (pz == ghostDualPointIndexRange[5])
    {
      nz = 2;
      if ((block->BoundaryBits & 32))
      {
        dz = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 32;
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
        cornerPoints[c << 2] = tmp[0] + lowerSpacing[0] * ((double)(px) + dx);
        cornerPoints[(c << 2) | 1] = tmp[1] + lowerSpacing[1] * ((double)(py) + dy);
        cornerPoints[(c << 2) | 2] = tmp[2] + lowerSpacing[2] * ((double)(pz) + dz);
      }
      else
      { // This could be the only degenerate path with a little extra cost.
        cornerPoints[c << 2] = tmp[0] + spacing[0] * (double)(1 << levelDiff) * ((double)(px) + dx);
        cornerPoints[(c << 2) | 1] =
          tmp[1] + spacing[1] * (double)(1 << levelDiff) * ((double)(py) + dy);
        cornerPoints[(c << 2) | 2] =
          tmp[2] + spacing[2] * (double)(1 << levelDiff) * ((double)(pz) + dz);
      }
    }
    else
    {
      // How do I chop the cells in half on the bondaries?
      // Move the tmp origin and change spacing.
      cornerPoints[c << 2] = tmp[0] + spacing[0] * ((double)(px) + dx);
      cornerPoints[(c << 2) | 1] = tmp[1] + spacing[1] * ((double)(py) + dy);
      cornerPoints[(c << 2) | 2] = tmp[2] + spacing[2] * ((double)(pz) + dz);
    }
  }

  // We have the points, now contour the cell.
  // Get edges.
  triCase = triCases + cubeCase;
  edge = triCase->edges;
  double pt[3];

  // Save the edge point ids in case we need to create a capping surface.
  vtkIdType edgePointIds[12]; // Is six the maximum?
  // For debugging
  // My capping permutations were giving me bad edges.
  // for( int ii = 0; ii < 12; ++ii)
  //  {
  //  edgePointIds[ii] = 0;
  //  }

  // loop over triangles
  while (*edge > -1)
  {
    // I want to avoid adding degenerate triangles.
    // Maybe the best way to do this is to have a point locator
    // merge points first.
    // Create brute force locator for a block, and resuse it.
    // Only permanently keep locator for edges shared between two blocks.
    for (int ii = 0; ii < 3; ++ii, ++edge) // insert triangle
    {
      vtkIdType* ptIdPtr = this->BlockLocator->GetEdgePointer(x, y, z, *edge);

      if (*ptIdPtr == -1)
      {
        // Compute the interpolation factor.
        v0 = cornerValues[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][0]];
        v1 = cornerValues[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][1]];
        k = (this->IsoValue - v0) / (v1 - v0);
        // I was trying to avoid sliver triangles
        // Moving the point to the corner caused non-manifold edges.
        // This caused surface artifacts.
        // if (k < vtkAMRDualContourEdgeLocatorMinTolerance)
        //  {
        //  k = vtkAMRDualContourEdgeLocatorMinTolerance;
        //  }
        // else if (k > vtkAMRDualContourEdgeLocatorMaxTolerance)
        //  {
        //  k = vtkAMRDualContourEdgeLocatorMaxTolerance;
        //  }
        // Add the point to the output and get the index of the point.
        int pt1Idx = (vtkAMRDualIsoEdgeToPointsTable[*edge][0] << 2);
        int pt2Idx = (vtkAMRDualIsoEdgeToPointsTable[*edge][1] << 2);
        // I wonder if this is any faster than incrementing a pointer.
        pt[0] = cornerPoints[pt1Idx] + k * (cornerPoints[pt2Idx] - cornerPoints[pt1Idx]);
        pt[1] =
          cornerPoints[pt1Idx | 1] + k * (cornerPoints[pt2Idx | 1] - cornerPoints[pt1Idx | 1]);
        pt[2] =
          cornerPoints[pt1Idx | 2] + k * (cornerPoints[pt2Idx | 2] - cornerPoints[pt1Idx | 2]);
        *ptIdPtr = this->Points->InsertNextPoint(pt);
        // Interpolate attributes
        // Find the offsets of the two attributes to interpolate
        vtkIdType offset0 = cornerOffsets[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][0]];
        vtkIdType offset1 = cornerOffsets[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][1]];
        this->InterpolateAttributes(block->Image, offset0, offset1, k, this->Mesh, *ptIdPtr);
      }
      edgePointIds[*edge] = pointIds[ii] = *ptIdPtr;
    }
    if (pointIds[0] != pointIds[1] && pointIds[0] != pointIds[2] && pointIds[1] != pointIds[2])
    {
      this->Faces->InsertNextCell(3, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
    }
  }

  if (this->EnableCapping)
  {
    this->CapCell(x, y, z, cubeBoundaryBits, cubeCase, edgePointIds, cornerPoints, cornerOffsets,
      blockId, block->Image);
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::AddCapPolygon(int ptCount, vtkIdType* pointIds, int blockId)
{
  if (this->TriangulateCap)
  {
    vtkIdType tri[3];
    int low = 1;
    int high = ptCount - 2;
    while (low <= high)
    {
      if (low == high)
      {
        tri[0] = pointIds[high + 1];
        tri[1] = pointIds[low - 1];
        tri[2] = pointIds[low];
        if (tri[0] != tri[1] && tri[0] != tri[2] && tri[1] != tri[2])
        {
          this->Faces->InsertNextCell(3, tri);
          this->BlockIdCellArray->InsertNextValue(blockId);
        }
      }
      else
      {
        tri[0] = pointIds[high + 1];
        tri[1] = pointIds[low - 1];
        tri[2] = pointIds[low];
        if (tri[0] != tri[1] && tri[0] != tri[2] && tri[1] != tri[2])
        {
          this->Faces->InsertNextCell(3, tri);
          this->BlockIdCellArray->InsertNextValue(blockId);
        }
        tri[0] = pointIds[high];
        tri[1] = pointIds[high + 1];
        tri[2] = pointIds[low];
        if (tri[0] != tri[1] && tri[0] != tri[2] && tri[1] != tri[2])
        {
          this->Faces->InsertNextCell(3, tri);
          this->BlockIdCellArray->InsertNextValue(blockId);
        }
      }
      ++low;
      --high;
    }
  }
  else
  {
    // Do not worry about degenerate polygons in this path.
    this->Faces->InsertNextCell(ptCount, pointIds);
    this->BlockIdCellArray->InsertNextValue(blockId);
  }
}

//----------------------------------------------------------------------------
// Now generate the capping surface.
// I chose to make a generic face case table. We decided to cap
// each face independently.  I permute the hex index into a face case
// and I permute the face corners and edges into hex corners and endges.
// It ends up being a little long to duplicate the code 6 times,
// but it is still fast.
void vtkAMRDualContour::CapCell(int cellX, int cellY, int cellZ, // cell index in block coordinates.
  // Which cell faces need to be capped.
  unsigned char cubeBoundaryBits,
  // Marching cubes case for this cell
  int cubeCase,
  // Ids of the point created on edges for the internal surface
  vtkIdType edgePointIds[12],
  // Locations of 8 corners (xyz4xyz4...); 4th value is not used.
  double cornerPoints[32],
  // The id order is VTK from marching cube cases.  Different than axis ordered "cornerPoints".
  vtkIdType cornerOffsets[8],
  // For block id array (for debugging).  I should just make this an ivar.
  int blockId,
  // For passing attributes to output mesh
  vtkDataSet* inData)
{
  int cornerIdx;
  vtkIdType* ptIdPtr;
  vtkIdType pointIds[6];
  // -X
  if ((cubeBoundaryBits & 1))
  {
    int faceCase =
      ((cubeCase & 1)) | ((cubeCase & 8) >> 2) | ((cubeCase & 128) >> 5) | ((cubeCase & 16) >> 1);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoNXCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNXCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }
  // +X
  if ((cubeBoundaryBits & 2))
  {
    int faceCase = ((cubeCase & 2) >> 1) | ((cubeCase & 32) >> 4) | ((cubeCase & 64) >> 4) |
      ((cubeCase & 4) << 1);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoPXCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPXCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }

  // -Y
  if ((cubeBoundaryBits & 4))
  {
    int faceCase =
      ((cubeCase & 1)) | ((cubeCase & 16) >> 3) | ((cubeCase & 32) >> 3) | ((cubeCase & 2) << 2);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoNYCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNYCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }

  // +Y
  if ((cubeBoundaryBits & 8))
  {
    int faceCase = ((cubeCase & 8) >> 3) | ((cubeCase & 4) >> 1) | ((cubeCase & 64) >> 4) |
      ((cubeCase & 128) >> 4);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoPYCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPYCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }

  // -Z
  if ((cubeBoundaryBits & 16))
  {
    int faceCase = (cubeCase & 1) | (cubeCase & 2) | (cubeCase & 4) | (cubeCase & 8);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoNZCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNZCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }

  // +Z
  if ((cubeBoundaryBits & 32))
  {
    int faceCase = ((cubeCase & 128) >> 7) | ((cubeCase & 64) >> 5) | ((cubeCase & 32) >> 3) |
      ((cubeCase & 16) >> 1);
    int* capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
    {
      int ptCount = 0;
      while (*capPtr >= 0)
      {
        if (*capPtr < 4)
        {
          cornerIdx = (vtkAMRDualIsoPZCapEdgeMap[*capPtr]);
          ptIdPtr = this->BlockLocator->GetCornerPointer(cellX, cellY, cellZ, cornerIdx);
          if (*ptIdPtr == -1)
          {
            *ptIdPtr = this->Points->InsertNextPoint(cornerPoints + (cornerIdx << 2));
            this->CopyAttributes(
              inData, cornerOffsets[vtkAMRDualLegacyIdToBitIdMap[cornerIdx]], this->Mesh, *ptIdPtr);
          }
          pointIds[ptCount++] = *ptIdPtr;
        }
        else
        {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPZCapEdgeMap[*capPtr]];
        }
        ++capPtr;
      }
      this->AddCapPolygon(ptCount, pointIds, blockId);
      if (*capPtr == -1)
      {
        ++capPtr;
      } // Skip to the next triangle.
    }
  }
}

//----------------------------------------------------------------------------
//  Note that the out attribute arrays must be preallocated to correct size.
template <class T>
double vtkDualGridContourInterpolateAttribute(T* inPtr, vtkIdType inId0, vtkIdType inId1, double k)
{
  double in0 = (double)(inPtr[inId0]);
  return in0 * (1.0 - k) + k * (double)(inPtr[inId1]);
}

//============================================================================
// Stuff for using input cell attributes to add output point attributes.
// I am not using the built in Attribute methods for duing this, although
// it is probably almost identical.

//----------------------------------------------------------------------------
void vtkAMRDualContour::InitializeCopyAttributes(vtkNonOverlappingAMR* hbdsInput, vtkDataSet* mesh)
{
  // Most of this is just getting a block with cell attributes so we can
  // call CopyAllocate.
  vtkCompositeDataIterator* iter = hbdsInput->NewIterator();
  iter->InitTraversal();
  if (iter->IsDoneWithTraversal())
  { // Empty input
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

//----------------------------------------------------------------------------
void vtkAMRDualContour::FinalizeCopyAttributes(vtkDataSet* mesh)
{
  mesh->GetPointData()->Squeeze();
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::InterpolateAttributes(vtkDataSet* uGrid, vtkIdType offset0,
  vtkIdType offset1, double k, vtkDataSet* mesh, vtkIdType outId)
{
  mesh->GetPointData()->InterpolateEdge(uGrid->GetCellData(), outId, offset0, offset1, k);
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::CopyAttributes(
  vtkDataSet* uGrid, vtkIdType inId, vtkDataSet* mesh, vtkIdType outId)
{
  mesh->GetPointData()->CopyData(uGrid->GetCellData(), inId, outId);
}
