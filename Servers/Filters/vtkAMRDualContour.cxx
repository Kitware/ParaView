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

#include "vtkstd/vector"

// Pipeline & VTK 
#include "vtkMarchingCubesCases.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
// PV interface
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkDataArraySelection.h"
// Data sets
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkAMRBox.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include <math.h>
#include <ctime>


vtkCxxRevisionMacro(vtkAMRDualContour, "1.1");
vtkStandardNewMacro(vtkAMRDualContour);




// It is working but we have some missing features.
// 1: Make a Clip Filter
// 2: Merge points.
// 3: Change degenerate quads to tris or remove.
// 4: Copy Attributes from input to output.
// 5: !!!!!!!!! Make sure we can disable parallel.


//----------------------------------------------------------------------------
// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkAMRDualContour::vtkAMRDualContour()
{
  this->IsoValue = 100.0;
  
  this->EnableDegenerateCells = 1;
  this->EnableCapping = 1;
  this->EnableMultiProcessCommunication = 1;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  // Pipeline
  this->SetNumberOfOutputPorts(1);
  

  this->BlockIdCellArray = 0;
  this->Helper = 0;
}

//----------------------------------------------------------------------------
vtkAMRDualContour::~vtkAMRDualContour()
{
}

//----------------------------------------------------------------------------
void vtkAMRDualContour::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO print state
  this->Superclass::PrintSelf(os,indent);

  os << indent << "IsoValue: " << this->IsoValue << endl;
}

//----------------------------------------------------------------------------
int vtkAMRDualContour::FillInputPortInformation(int /*port*/,
                                                    vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRDualContour::FillOutputPortInformation(int port, vtkInformation *info)
{
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      assert( 0 && "Invalid output port." );
      break;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRDualContour::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the data set which we are to process
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *hbdsInput=vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the outputs
  // 0
  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  mbdsOutput0->SetNumberOfBlocks(1);
  vtkMultiPieceDataSet *mpds=vtkMultiPieceDataSet::New();
  mbdsOutput0->SetBlock(0,mpds);

  mpds->SetNumberOfPieces(0);

  if ( hbdsInput==0 )
    {
    // Do not deal with rectilinear grid
    vtkErrorMacro("This filter requires a vtkHierarchicalBoxDataSet on its input.");
    return 0;
    }

  // This is a lot to go through to get the name of the array to process.
  vtkInformationVector *inArrayVec = this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
    {
    vtkErrorMacro("Problem finding array to process");
    return 0;
    }
  vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);
  if (!inArrayInfo)
    {
    vtkErrorMacro("Problem getting name of array to process.");
    return 0;
    }
  if ( ! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
    {
    vtkErrorMacro("Missing field name.");
    return 0;
    }
  const char *arrayNameToProcess = inArrayInfo->Get(vtkDataObject::FIELD_NAME());      


  this->Helper = vtkAMRDualGridHelper::New();
  this->Helper->SetEnableDegenerateCells(this->EnableDegenerateCells);
  this->Helper->SetEnableMultiProcessCommunication(this->EnableMultiProcessCommunication);
  this->Helper->Initialize(hbdsInput, arrayNameToProcess);

  vtkPolyData* mesh = vtkPolyData::New();
  this->Points = vtkPoints::New();
  this->Faces = vtkCellArray::New();
  mesh->SetPoints(this->Points);
  mesh->SetPolys(this->Faces);
  mpds->SetPiece(0, mesh);
  
  // For debugging.
  this->BlockIdCellArray = vtkIntArray::New();
  this->BlockIdCellArray->SetName("BlockIds");
  mesh->GetCellData()->AddArray(this->BlockIdCellArray);

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
      this->ProcessBlock(block, blockId);
      }
    }

  this->BlockIdCellArray->Delete();
  this->BlockIdCellArray = 0;

  mesh->Delete();
  this->Points->Delete();
  this->Points = 0;
  this->Faces->Delete();
  this->Faces = 0;

  mpds->Delete();
  this->Helper->Delete();
  this->Helper = 0;

  return 1;
}

//----------------------------------------------------------------------------
// The only data specific stuff we need to do for the contour.
template <class T>
void vtkDualGridContourExtractCornerValues(
  T* ptr, int yInc, int zInc,
  double values[8]) 
{
  // Because of the marching cubes case table, I am stuck with
  // VTK's indexing of corners.
  values[0] = (double)(*ptr);
  values[1] = (double)(ptr[1]);
  values[2] = (double)(ptr[1+yInc]);
  values[3] = (double)(ptr[yInc]);
  values[4] = (double)(ptr[zInc]);
  values[5] = (double)(ptr[1+zInc]);
  values[6] = (double)(ptr[1+yInc+zInc]);
  values[7] = (double)(ptr[yInc+zInc]);
}
//----------------------------------------------------------------------------
void vtkAMRDualContour::ProcessBlock(vtkAMRDualGridHelperBlock* block,
                                     int blockId)
{
  vtkImageData* image = block->Image;
  if (image == 0)
    { // Remote blocks are only to setup local block bit flags.
    return;
    }
  vtkDataArray *volumeFractionArray = this->GetInputArrayToProcess(0, image);
  void* volumeFractionPtr = volumeFractionArray->GetVoidPointer(0);
  double  origin[3];
  double* spacing;
  int     extent[6];
  
  // Get the origin and point extent of the dual grid (with ghost level).
  // This is the same as the cell extent of the original grid (with ghosts).
  image->GetExtent(extent);
  --extent[1];
  --extent[3];
  --extent[5];

  image->GetOrigin(origin);
  spacing = image->GetSpacing();
  // Dual cells are shifted half a pixel.
  origin[0] += 0.5 * spacing[0];
  origin[1] += 0.5 * spacing[1];
  origin[2] += 0.5 * spacing[2];
  
  // We deal with the various data types by copying the corner values
  // into a double array.  We have to cast anyway to compute the case.
  double cornerValues[8];

  // The templated function needs the increments for pointers 
  // cast to the correct datatype.
  int yInc = (extent[1]-extent[0]+1);
  int zInc = yInc * (extent[3]-extent[2]+1);
  // Use void pointers to march through the volume before we cast.
  int dataType = volumeFractionArray->GetDataType();
  int xVoidInc = volumeFractionArray->GetDataTypeSize();
  int yVoidInc = xVoidInc * yInc;
  int zVoidInc = xVoidInc * zInc;

  int cubeIndex;

  // Loop over all the cells in the dual grid.
  int x, y, z;
  // These are needed to handle the cropped boundary cells.
  double ox, oy, oz;
  double sx, sy, sz;
  int xMax = extent[1]-1;
  int yMax = extent[3]-1;
  int zMax = extent[5]-1;
  //-
  unsigned char* zPtr = (unsigned char*)(volumeFractionPtr);
  unsigned char* yPtr;
  unsigned char* xPtr;
  for (z = extent[4]; z < extent[5]; ++z)
    {
    int nz = 1;
    if (z == extent[4]) {nz = 0;}
    else if (z == zMax) {nz = 2;}
    sz = spacing[2];
    oz = origin[2] + (double)(z)*sz;
    yPtr = zPtr;
    for (y = extent[2]; y < extent[3]; ++y)
      {
      int ny = 1;
      if (y == extent[2]) {ny = 0;}
      else if (y == yMax) {ny = 2;}
      sy = spacing[1];
      oy = origin[1] + (double)(y)*sy;
      xPtr = yPtr;
      for (x = extent[0]; x < extent[1]; ++x)
        {
        int nx = 1;
        if (x == extent[0]) {nx = 0;}
        else if (x == xMax) {nx = 2;}
        sx = spacing[0];
        ox = origin[0] + (double)(x)*sx;
        // Skip the cell if a neighbor is already processing it.
        if ( (block->RegionBits[nx][ny][nz] & vtkAMRRegionBitOwner) )
          {
          // Get the corner values as doubles
          switch (dataType)
            {
            vtkTemplateMacro(vtkDualGridContourExtractCornerValues(
                             (VTK_TT *)(xPtr), yInc, zInc,
                             cornerValues));
            default:
              vtkGenericWarningMacro("Execute: Unknown ScalarType");
            }
          // compute the case index
          cubeIndex = 0;
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
          this->ProcessDualCell(block, blockId,
                                cubeIndex, x, y, z,
                                cornerValues);
          }
        xPtr += xVoidInc;
        }
      yPtr += yVoidInc;
      }
    zPtr += zVoidInc;
    }
}

static int vtkAMRDualIsoEdgeToPointsTable[12][2] =
  { {0,1}, {1,3}, {2,3}, {0,2},
    {4,5}, {5,7}, {6,7}, {4,6},
    {0,4}, {1,5}, {2,6}, {3,7}};
static int vtkAMRDualIsoEdgeToVTKPointsTable[12][2] =
  { {0,1}, {1,2}, {3,2}, {0,3},
    {4,5}, {5,6}, {7,6}, {4,7},
    {0,4}, {1,5}, {3,7}, {2,6}};





// Generic table for clipping a square.
// We can have two polygons.
// Polygons are separated by -1, and terminated by -2.
// 0-3 are the corners of the square (00,10,11,01).
// 4-7 are for points on the edges (4:(00-10),5:(10-11),6:(11-01),7:(01-00)
static int vtkAMRDualIsoCappingTable[16][8] =
  { {-2,0,0,0,0,0,0,0},  //(0000)
    {0,4,7,-2,0,0,0,0},  //(1000)
    {1,5,4,-2,0,0,0,0},  //(0100)
    {0,1,5,7,-2,0,0,0},  //(1100)
    {2,6,5,-2,0,0,0,0},  //(0010)
    {0,4,7,-1,2,6,5,-2}, //(1010)
    {1,2,6,4,-2,0,0,0},  //(0110)
    {0,1,2,6,7,-2,0,0},  //(1110)
    {3,7,6,-2,0,0,0,0},  //(0001)
    {3,0,4,6,-2,0,0,0},  //(1001)
    {1,5,4,-1,3,7,6,-2}, //(0101)
    {3,0,1,5,6,-2,0,0},  //(1101)
    {2,3,7,5,-2,0,0,0},  //(0011)
    {2,3,0,4,5,-2,0,0},  //(1011)
    {1,2,3,7,4,-2,0,0},  //(0111)
    {0,1,2,3,-2,0,0,0}}; //(1111)

// These tables map the corners and edges from the above table
// into corners and edges for the faces of a cube.
// First for map 0-3 into corners 0-7 000,100,010,110,001,101,011,111
// Edges 4-7 get mapped to standard cube edge index.
// 0:(000-100),1:(100-110),2:(110-010),3:(010-000),
static int vtkAMRDualIsoNXCapEdgeMap[8] = {0,2,6,4 ,3,10,7,8};
static int vtkAMRDualIsoPXCapEdgeMap[8] = {1,5,7,3 ,9,5,11,1};

static int vtkAMRDualIsoNYCapEdgeMap[8] = {0,4,5,1 ,8,4,9,0};
static int vtkAMRDualIsoPYCapEdgeMap[8] = {2,3,7,6 ,2,11,6,10};

static int vtkAMRDualIsoNZCapEdgeMap[8] = {0,1,3,2 ,0,1,2,3};
static int vtkAMRDualIsoPZCapEdgeMap[8] = {6,7,5,4 ,6,5,4,7};


//----------------------------------------------------------------------------
// Not implemented as optimally as we could.  It can be improved by making
// a fast path for internal cells (with no degeneracies).
void vtkAMRDualContour::ProcessDualCell(
  vtkAMRDualGridHelperBlock* block, int blockId,
  int cubeCase,
  int x, int y, int z,
  double cornerValues[8])
{
  // I am trying to exit as quick as possible if there is
  // no surface to generate.  I could also check that the index
  // is not on boundary.
  if (cubeCase == 0 || (cubeCase == 255 && block->BoundaryBits == 0))
    {
    return;
    }

  // Which boundaries does this cube/cell touch?
  unsigned char cubeBoundaryBits = 0;
  // If this cell is degenerate, then remove triangles with 2 points.
  int degenerateFlag = 0;  
  
  int nx, ny, nz; // Neighbor index [3][3][3];
  vtkIdType pointIds[6];
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST  *edge;
  double k, v0, v1;
  triCases =  vtkMarchingCubesTriangleCases::GetCases();
  int *dims = block->Image->GetDimensions();
  int yInc = dims[0]-1; // -1 for point to cell
  int zInc = yInc * (dims[1]-1);

  // Compute the spacing fro this level and one lower level;
  const double *tmp = this->Helper->GetRootSpacing();
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
  ghostDualPointIndexRange[1] += block->OriginIndex[0]-1;
  ghostDualPointIndexRange[2] += block->OriginIndex[1];
  ghostDualPointIndexRange[3] += block->OriginIndex[1]-1;
  ghostDualPointIndexRange[4] += block->OriginIndex[2];
  ghostDualPointIndexRange[5] += block->OriginIndex[2]-1;
  // Change to global index.
  x += block->OriginIndex[0];
  y += block->OriginIndex[1];
  z += block->OriginIndex[2];

  double dx, dy, dz; // Chop cells in half at boundary.
  double cornerPoints[32]; // 4 is easier to optimize than 3.
  // Loop over the corners.
  for (int c = 0; c < 8; ++c)
    {
    // The varibles dx,dy,dz handle boundary cells. 
    // They shift point by half a pixel on the boundary.
    dx = dy = dz = 0.5;
    // Place the point in one of the 26 ghost regions.
    int px, py, pz; // Corner global xyz index.
    // CornerIndex
    px =(c & 1)?x+1:x;
    if (px == ghostDualPointIndexRange[0]) 
      {
      nx = 0;
      if ( (block->BoundaryBits & 1) ) 
        {
        dx = 1.0;
        cubeBoundaryBits = cubeBoundaryBits | 1;
        }
      }
    else if (px == ghostDualPointIndexRange[1]) 
      {
      nx = 2;
      if ( (block->BoundaryBits & 2) )
        {
        dx = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 2;
        }
      }
    else {nx = 1;}
    py =(c & 2)?y+1:y;
    if (py == ghostDualPointIndexRange[2]) 
      {
      ny = 0;
      if ( (block->BoundaryBits & 4) ) 
        {
        dy = 1.0;
        cubeBoundaryBits = cubeBoundaryBits | 4;
        }
      }
    else if (py == ghostDualPointIndexRange[3]) 
      {
      ny = 2;
      if ( (block->BoundaryBits & 8) ) 
        {
        dy = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 8;
        }
      }
    else {ny = 1;}
    pz =(c & 4)?z+1:z;
    if (pz == ghostDualPointIndexRange[4]) 
      {
      nz = 0;
      if ( (block->BoundaryBits & 16) )
        {
        dz = 1.0;
        cubeBoundaryBits = cubeBoundaryBits | 16;
        }
      }
    else if (pz == ghostDualPointIndexRange[5]) 
      {
      nz = 2;
      if ( (block->BoundaryBits & 32) )
        {
        dz = 0.0;
        cubeBoundaryBits = cubeBoundaryBits | 32;
        }
      }
    else {nz = 1;}
    
    if (block->RegionBits[nx][ny][nz] & vtkAMRRegionBitsDegenerateMask)
      { // point lies in lower level neighbor.
      degenerateFlag = 1;
      int levelDiff = block->RegionBits[nx][ny][nz] & vtkAMRRegionBitsDegenerateMask;
      px = px >> levelDiff;
      py = py >> levelDiff;
      pz = pz >> levelDiff;
      // Shift half a pixel to get center of cell (dual point).
      if (levelDiff == 1)
        { // This is the most common case; avoid extra multiplications.
        cornerPoints[c<<2]     = tmp[0] + lowerSpacing[0] * ((double)(px)+dx);
        cornerPoints[(c<<2)|1] = tmp[1] + lowerSpacing[1] * ((double)(py)+dy);
        cornerPoints[(c<<2)|2] = tmp[2] + lowerSpacing[2] * ((double)(pz)+dz);
        }
      else
        { // This could be the only degenerate path with a little extra cost.
        cornerPoints[c<<2]     = tmp[0] + spacing[0] * (double)(1 << levelDiff) * ((double)(px)+dx);
        cornerPoints[(c<<2)|1] = tmp[1] + spacing[1] * (double)(1 << levelDiff) * ((double)(py)+dy);
        cornerPoints[(c<<2)|2] = tmp[2] + spacing[2] * (double)(1 << levelDiff) * ((double)(pz)+dz);
        }
      }
    else
      {
      // How do I chop the cells in half on the bondaries?
      // Move the tmp origin and change spacing.
      cornerPoints[c<<2]     = tmp[0] + spacing[0] * ((double)(px)+dx); 
      cornerPoints[(c<<2)|1] = tmp[1] + spacing[1] * ((double)(py)+dy); 
      cornerPoints[(c<<2)|2] = tmp[2] + spacing[2] * ((double)(pz)+dz); 
      }
    }
  // We have the points, now contour the cell.
  // Get edges.
  triCase = triCases + cubeCase;
  edge = triCase->edges; 
  double pt[3];

  // Save the edge point ids incase we need to create a capping surface.
  vtkIdType edgePointIds[12]; // Is six the maximum?
  // For debugging
  // My capping permutations were giving me bad edges.
  //for( int ii = 0; ii < 12; ++ii)
  //  {
  //  edgePointIds[ii] = 0;
  //  }

  // loop over triangles  
  while(*edge > -1)
    {
    // I want to avoid adding degenerate triangles. 
    // Maybe the best way to do this is to have a point locator
    // merge points first.
    // Create brute force locator for a block, and resuse it.
    // Only permanently keep locator for edges shared between two blocks.
    for (int ii=0; ii<3; ++ii, ++edge) //insert triangle
      {
      // Compute the interpolation factor.
      v0 = cornerValues[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][0]];
      v1 = cornerValues[vtkAMRDualIsoEdgeToVTKPointsTable[*edge][1]];
      k = (this->IsoValue-v0) / (v1-v0);
      // Add the point to the output and get the index of the point.
      int pt1Idx = (vtkAMRDualIsoEdgeToPointsTable[*edge][0]<<2);
      int pt2Idx = (vtkAMRDualIsoEdgeToPointsTable[*edge][1]<<2);
      // I wonder if this is any faster than incrementing a pointer.
      pt[0] = cornerPoints[pt1Idx] + k*(cornerPoints[pt2Idx]-cornerPoints[pt1Idx]);
      pt[1] = cornerPoints[pt1Idx|1] + k*(cornerPoints[pt2Idx|1]-cornerPoints[pt1Idx|1]);
      pt[2] = cornerPoints[pt1Idx|2] + k*(cornerPoints[pt2Idx|2]-cornerPoints[pt1Idx|2]);
      edgePointIds[*edge] = pointIds[ii] = this->Points->InsertNextPoint(pt);
      }

    this->Faces->InsertNextCell(3, pointIds);
    this->BlockIdCellArray->InsertNextValue(blockId);
    }

  if (this->EnableCapping)
    {
    this->CapCell(cubeBoundaryBits, cubeCase, edgePointIds, cornerPoints, blockId);
    }
}

//----------------------------------------------------------------------------
// Now generate the capping surface.
// I chose to make a generic face case table. We decided to cap 
// each face independantly.  I permute the hex index into a face case
// and I permute the face corners and edges into hex corners and endges.
// It endsup being a little long to duplicate the code 6 times,
// but it is still fast.
void vtkAMRDualContour::CapCell(
  // Which cell faces need to be capped.
  unsigned char cubeBoundaryBits,
  // Marching cubes case for this cell
  int cubeCase,
  // Ids of the point created on edges for the internal surface
  vtkIdType edgePointIds[12],
  // Locations of 8 corners (xyz4xyz4...); 4th value is not used.
  double cornerPoints[32],
  // For block id array (for debugging).  I should just make this an ivar.
  int blockId)
{
  vtkIdType pointIds[6];
  // -X
  if ( (cubeBoundaryBits & 1))
    {
    int faceCase = ((cubeCase&1)) | ((cubeCase&8)>>2) | ((cubeCase&128)>>5) | ((cubeCase&16)>>1);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoNXCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNXCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }
  // +X
  if ( (cubeBoundaryBits & 2))
    {
    int faceCase = ((cubeCase&2)>>1) | ((cubeCase&32)>>4) | ((cubeCase&64)>>4) | ((cubeCase&4)<<1);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoPXCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPXCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }
    
  // -Y
  if ( (cubeBoundaryBits & 4))
    {
    int faceCase = ((cubeCase&1)) | ((cubeCase&16)>>3) | ((cubeCase&32)>>3) | ((cubeCase&2)<<2);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoNYCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNYCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }
    
  // +Y
  if ( (cubeBoundaryBits & 8))
    {
    int faceCase = ((cubeCase&8)>>3) | ((cubeCase&4)>>1) | ((cubeCase&64)>>4) | ((cubeCase&128)>>4);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoPYCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPYCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }
    
    
  // -Z
  if ( (cubeBoundaryBits & 16))
    {
    int faceCase = (cubeCase&1) | (cubeCase&2) | (cubeCase&4) | (cubeCase&8);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoNZCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoNZCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }

  // +Z
  if ( (cubeBoundaryBits & 32))
    {
    int faceCase = ((cubeCase&128)>>7) | ((cubeCase&64)>>5) | ((cubeCase&32)>>3) | ((cubeCase&16)>>1);
    int *capPtr = vtkAMRDualIsoCappingTable[faceCase];
    while (*capPtr != -2)
      {
      int ptCount = 0;
      while (*capPtr >= 0)
        {
        if (*capPtr < 4)
          {
          pointIds[ptCount++] = this->Points->InsertNextPoint(
            cornerPoints+((vtkAMRDualIsoPZCapEdgeMap[*capPtr])<<2));
          }
        else
          {
          pointIds[ptCount++] = edgePointIds[vtkAMRDualIsoPZCapEdgeMap[*capPtr]];
          }
        ++capPtr;
        }
      this->Faces->InsertNextCell(ptCount, pointIds);
      this->BlockIdCellArray->InsertNextValue(blockId);
      if (*capPtr == -1) {++capPtr;} // Skip to the next triangle.
      }
    }
}




