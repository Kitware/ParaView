/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHData.h"

#include "vtkStructuredData.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIntArray.h"
#include "vtkLargeInteger.h"
#include "vtkLine.h"
#include "vtkLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkCTHData, "1.2");
vtkStandardNewMacro(vtkCTHData);

//----------------------------------------------------------------------------
vtkCTHData::vtkCTHData()
{
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();
  
  this->DataDescription = VTK_EMPTY;
  
  this->BlockOrigins = vtkFloatArray::New();
  this->BlockOrigins->SetNumberOfComponents(3);
  this->BlockSpacings = vtkFloatArray::New();
  this->BlockSpacings->SetNumberOfComponents(3);

  this->NumberOfGhostLevels = 0;
}

//----------------------------------------------------------------------------
vtkCTHData::~vtkCTHData()
{
  this->Vertex->Delete();
  this->Line->Delete();
  this->Pixel->Delete();
  this->Voxel->Delete();

  this->BlockOrigins->Delete();
  this->BlockOrigins = NULL;
  this->BlockSpacings->Delete();
  this->BlockSpacings = NULL;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlock(int blockId, vtkImageData* block)
{
  float *tmp;
  int numCellsPerBlock = this->GetNumberOfCellsPerBlock();
  int numPointsPerBlock = this->GetNumberOfPointsPerBlock();

  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block id out of range.");
    return;
    }

  block->Initialize();
  block->SetDimensions(this->Dimensions);
  tmp = this->GetBlockSpacing(blockId);
  block->SetSpacing(tmp);
  tmp = this->GetBlockOrigin(blockId);
  block->SetOrigin(tmp);

  // Copy arrays.
  int num, idx, tupleSize;
  vtkDataArray *array;
  unsigned char *ptr1;
  vtkDataArray *newArray;
  unsigned char *ptr2;

  num = this->PointData->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    array = this->PointData->GetArray(idx);
    newArray = array->NewInstance();
    newArray->SetNumberOfComponents(array->GetNumberOfComponents());
    newArray->SetNumberOfTuples(numPointsPerBlock);
    newArray->SetName(array->GetName());
    // Copy memory segment.
    tupleSize = array->GetDataTypeSize() * array->GetNumberOfComponents();
    ptr1 = (unsigned char*)(array->GetVoidPointer(0));
    ptr2 = (unsigned char*)(newArray->GetVoidPointer(0));
    ptr1 += blockId * tupleSize * numPointsPerBlock;
    memcpy(ptr2, ptr1, tupleSize*numPointsPerBlock);
    block->GetPointData()->AddArray(newArray);
    newArray->Delete();
    newArray = NULL;
    }

  num = this->CellData->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    array = this->CellData->GetArray(idx);
    newArray = array->NewInstance();
    newArray->SetNumberOfComponents(array->GetNumberOfComponents());
    newArray->SetNumberOfTuples(numCellsPerBlock);
    newArray->SetName(array->GetName());
    // Copy memory segment.
    tupleSize = array->GetDataTypeSize() * array->GetNumberOfComponents();
    ptr1 = (unsigned char*)(array->GetVoidPointer(0));
    ptr2 = (unsigned char*)(newArray->GetVoidPointer(0));
    ptr1 += blockId * tupleSize * numCellsPerBlock;
    memcpy(ptr2, ptr1, tupleSize*numCellsPerBlock);
    block->GetCellData()->AddArray(newArray);
    newArray->Delete();
    newArray = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkCTHData::SetNumberOfBlocks(int num)
{
  float* ptr;

  if (this->GetNumberOfBlocks() == num)
    {
    return;
    }
  
  this->Modified();
  this->BlockOrigins->SetNumberOfTuples(num);
  this->BlockSpacings->SetNumberOfTuples(num);

  ptr = this->BlockOrigins->GetPointer(0);
  memset(ptr, 0, num*3);
  ptr = this->BlockSpacings->GetPointer(0);
  memset(ptr, 0, num*3);
}

//----------------------------------------------------------------------------
int vtkCTHData::GetNumberOfBlocks()
{
  return this->BlockOrigins->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockOrigin(int blockId, float ox, float oy, float oz)
{
  float origin[3];

  origin[0] = ox;
  origin[1] = oy;
  origin[2] = oz;
  this->SetBlockOrigin(blockId, origin); 
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockOrigin(int blockId, float* origin)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return;
    }
  float *tmp;
  tmp = this->GetBlockOrigin(blockId);
  if (tmp[0] == origin[0] && tmp[1] == origin[1] && tmp[2] == origin[2])
    {
    return;
    }
  this->Modified();
  this->BlockOrigins->SetTuple(blockId, origin);
}


//----------------------------------------------------------------------------
float* vtkCTHData::GetBlockOrigin(int blockId)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return NULL;
    }
  return this->BlockOrigins->GetTuple(blockId);
}


//----------------------------------------------------------------------------
void vtkCTHData::SetBlockSpacing(int blockId, float sx, float sy, float sz)
{
  float spacing[3];

  spacing[0] = sx;
  spacing[1] = sy;
  spacing[2] = sz;
  this->SetBlockSpacing(blockId, spacing); 
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockSpacing(int blockId, float* spacing)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return;
    }
  float *tmp;
  tmp = this->GetBlockSpacing(blockId);
  if (tmp[0] == spacing[0] && tmp[1] == spacing[1] && tmp[2] == spacing[2])
    {
    return;
    }
  this->Modified();
  this->BlockSpacings->SetTuple(blockId, spacing);
}

//----------------------------------------------------------------------------
float* vtkCTHData::GetBlockSpacing(int blockId)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return NULL;
    }
  return this->BlockSpacings->GetTuple(blockId);
}



//----------------------------------------------------------------------------
void vtkCTHData::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  int pointsPerBlock = this->GetNumberOfPointsPerBlock();
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  int offset = pointsPerBlock * blockId;
  int num, idx;
  vtkIdType id;

  vtkStructuredData::GetCellPoints(blockCellId,ptIds,this->DataDescription,
                                   this->GetDimensions());

  // Shift the ids;
  num = ptIds->GetNumberOfIds();
  for (idx = 0; idx < num; ++idx)
    {
    id = ptIds->GetId(idx);
    id += offset;
    ptIds->SetId(idx, id);
    }
}

//----------------------------------------------------------------------------
void vtkCTHData::GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
{
  // Which block is this point in?
  vtkIdType blockId = ptId / this->GetNumberOfPointsPerBlock();
  vtkIdType numCellsPerBlock = this->GetNumberOfCellsPerBlock();
  vtkIdType* ptr;
  vtkIdType num, id;
  
  vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());
  ptr = cellIds->GetPointer(0);
  num = cellIds->GetNumberOfIds();
  for (id = 0; id < num; ++id)
    {
    *ptr += numCellsPerBlock * blockId;
    ++ptr;
    }
}



//----------------------------------------------------------------------------
void vtkCTHData::GetPoint(vtkIdType id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}



//----------------------------------------------------------------------------
vtkIdType vtkCTHData::GetNumberOfPointsPerBlock()
{
  int *dims = this->GetDimensions();
  return dims[0]*dims[1]*dims[2];
}


//----------------------------------------------------------------------------
vtkIdType vtkCTHData::GetNumberOfPoints()
{
  return this->GetNumberOfPointsPerBlock() * this->GetNumberOfBlocks();
}


//----------------------------------------------------------------------------
int vtkCTHData::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}



//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points 
// object.
void vtkCTHData::CopyStructure(vtkDataSet *ds)
{
  int i;
  vtkCTHData *src=vtkCTHData::SafeDownCast(ds);
  this->Initialize();

  this->SetNumberOfBlocks(src->GetNumberOfBlocks());
  for (i = 0; i < src->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockOrigin(i, src->GetBlockOrigin(i));  
    this->SetBlockSpacing(i, src->GetBlockSpacing(i));  
    }

  for (i = 0; i < 3; ++i)
    {
    this->Dimensions[i] = src->Dimensions[i];
    this->Extent[i] = src->Extent[i];
    this->Extent[i+3] = src->Extent[i+3];
    }


  this->DataDescription = src->DataDescription;
  this->CopyInformation(src);
}


//----------------------------------------------------------------------------
// The input data object must be of type vtkCTHData or a subclass!
void vtkCTHData::CopyTypeSpecificInformation( vtkDataObject *data )
{
  vtkCTHData *cth = vtkCTHData::SafeDownCast(data);

  // Copy the generic stuff
  this->CopyInformation( data );
  
  // Now do the specific stuff
  this->SetNumberOfBlocks(cth->GetNumberOfBlocks());
  for (int i = 0; i < cth->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockOrigin(i, cth->GetBlockOrigin(i));  
    this->SetBlockSpacing(i, cth->GetBlockSpacing(i));  
    this->Extent[i] = cth->Extent[i];
    this->Extent[i+3] = cth->Extent[i+3];
    this->Dimensions[i] = cth->Dimensions[i];
    }
}


//----------------------------------------------------------------------------

vtkCell *vtkCTHData::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  float x[3];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  float *origin = this->GetBlockOrigin(blockId);
  float *spacing = this->GetBlockSpacing(blockId);

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from empty CTH data.");
    return NULL;
    }
  
  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      //cell = this->EmptyCell;
      return NULL;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      iMin = blockCellId;
      iMax = blockCellId + 1;
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      jMin = blockCellId;
      jMax = blockCellId + 1;
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      kMin = blockCellId;
      kMax = blockCellId + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = blockCellId / (dims[0]-1);
      jMax = jMin + 1;
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      jMin = blockCellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = blockCellId / (dims[1]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = blockCellId / (dims[0]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMin = blockCellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (blockCellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = blockCellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell = this->Voxel;
      break;
    }

  // Extract point coordinates and point ids
  // Ids are relative to extent min.
  npts = 0;
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType npts, idx;
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockPointOffset;
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  float *origin = this->GetBlockOrigin(blockId);
  float *spacing = this->GetBlockSpacing(blockId);
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    cell->SetCellTypeToEmptyCell();
    return;
    }
  
  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      cell->SetCellTypeToEmptyCell();
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell->SetCellTypeToVertex();
      break;

    case VTK_X_LINE:
      iMin = blockCellId;
      iMax = blockCellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Y_LINE:
      jMin = blockCellId;
      jMax = blockCellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Z_LINE:
      kMin = blockCellId;
      kMax = blockCellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_XY_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = blockCellId / (dims[0]-1);
      jMax = jMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_YZ_PLANE:
      jMin = blockCellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = blockCellId / (dims[1]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XZ_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = blockCellId / (dims[0]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XYZ_GRID:
      iMin = blockCellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (blockCellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = blockCellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell->SetCellTypeToVoxel();
      break;
    }

  // Extract point coordinates and point ids
  blockPointOffset = this->GetNumberOfPointsPerBlock() * blockId;
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

        idx = blockPointOffset + loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }
}


//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkCTHData::GetCellBounds(vtkIdType cellId, float bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  float x[3];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  float *origin = this->GetBlockOrigin(blockId);
  float *spacing = this->GetBlockSpacing(blockId);
  int *dims = this->GetDimensions();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting cell bounds from an empty image.");
    bounds[0] = bounds[1] = bounds[2] = bounds[3] 
      = bounds[4] = bounds[5] = 0.0;
    return;
    }
  
  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return;

    case VTK_SINGLE_POINT: // blockCellId can only be = 0
      break;

    case VTK_X_LINE:
      iMin = blockCellId;
      iMax = blockCellId + 1;
      break;

    case VTK_Y_LINE:
      jMin = blockCellId;
      jMax = blockCellId + 1;
      break;

    case VTK_Z_LINE:
      kMin = blockCellId;
      kMax = blockCellId + 1;
      break;

    case VTK_XY_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = blockCellId / (dims[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      jMin = blockCellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = blockCellId / (dims[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      iMin = blockCellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = blockCellId / (dims[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = blockCellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (blockCellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = blockCellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      break;
    }


  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
  
  // Extract point coordinates
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
    bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
    {
    x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
    bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
    bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
    }
  for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
    {
    x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 
    bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
    bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
    }
}

//----------------------------------------------------------------------------
float *vtkCTHData::GetPoint(vtkIdType ptId)
{
  static float x[3];
  int i, loc[3];
  int pointsPerBlock = this->GetNumberOfPointsPerBlock();
  int blockId = ptId / pointsPerBlock;
  ptId = ptId - (blockId*pointsPerBlock);
  float *origin = this->GetBlockOrigin(blockId);
  float *spacing = this->GetBlockSpacing(blockId);
  int *dims = this->GetDimensions();

  x[0] = x[1] = x[2] = 0.0;
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a point from an empty image.");
    return x;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      return x;

    case VTK_SINGLE_POINT: 
      loc[0] = loc[1] = loc[2] = 0;
      break;

    case VTK_X_LINE:
      loc[1] = loc[2] = 0;
      loc[0] = ptId;
      break;

    case VTK_Y_LINE:
      loc[0] = loc[2] = 0;
      loc[1] = ptId;
      break;

    case VTK_Z_LINE:
      loc[0] = loc[1] = 0;
      loc[2] = ptId;
      break;

    case VTK_XY_PLANE:
      loc[2] = 0;
      loc[0] = ptId % dims[0];
      loc[1] = ptId / dims[0];
      break;

    case VTK_YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % dims[1];
      loc[2] = ptId / dims[1];
      break;

    case VTK_XZ_PLANE:
      loc[1] = 0;
      loc[0] = ptId % dims[0];
      loc[2] = ptId / dims[0];
      break;

    case VTK_XYZ_GRID:
      loc[0] = ptId % dims[0];
      loc[1] = (ptId / dims[0]) % dims[1];
      loc[2] = ptId / (dims[0]*dims[1]);
      break;
    }

  for (i=0; i<3; i++)
    {
    x[i] = origin[i] + (loc[i]+this->Extent[i*2]) * spacing[i];
    }

  return x;
}

//----------------------------------------------------------------------------
// Find the closest point.
vtkIdType vtkCTHData::FindPoint(float x[3])
{
  int numPointsPerBlock = this->GetNumberOfPointsPerBlock();
  int blockId, numBlocks, bestBlock, bestBlockPtId;
  int i, loc[3];
  float d;
  float dist, bestDist;
  float *origin;
  float *spacing;
  int *dims = this->GetDimensions();

  // But force.
  // Loop through all blocks.
  bestDist = VTK_LARGE_FLOAT;
  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = this->GetBlockOrigin(blockId);
    spacing = this->GetBlockSpacing(blockId);
    dist = 0.0;
    //
    //  Compute the ijk location
    //
    for (i=0; i<3; i++) 
      {
      d = x[i] - origin[i];
      loc[i] = (int) ((d / spacing[i]) + 0.5);
      if ( loc[i] < this->Extent[i*2])
        {
        loc[i] = this->Extent[i*2];
        }
      if (loc[i] > this->Extent[i*2+1])
        {
        loc[i] = this->Extent[i*2+1];
        }
      d = d - (spacing[i]*loc[i]);
      dist += d*d;
      // since point id is relative to the first point actually stored
      loc[i] -= this->Extent[i*2];
      }
    if (dist < bestDist)
      {
      bestBlock = blockId;
      bestBlockPtId = loc[2]*dims[0]*dims[1] + loc[1]*dims[0] + loc[0];
      bestDist = dist;
      }
    }
  return bestBlock*numPointsPerBlock + bestBlockPtId;
}

//----------------------------------------------------------------------------
vtkIdType vtkCTHData::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId), 
                                  float vtkNotUsed(tol2), 
                                  int& subId, float pcoords[3], 
                                  float *weights)
{
  return
    this->FindCell( x, (vtkCell *)NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkCTHData::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkIdType vtkNotUsed(cellId),
                                 float vtkNotUsed(tol2), 
                                 int& subId, float pcoords[3], float *weights)
{
  int loc[3];
  int *dims = this->GetDimensions();
  int blockId, numBlocks;

  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    if ( this->ComputeStructuredCoordinates(blockId, x, loc, pcoords) != 0 )
      {
      vtkVoxel::InterpolationFunctions(pcoords,weights);
      //
      //  From this location get the cell id
      //
      subId = 0;
      return (this->GetNumberOfCellsPerBlock() * blockId) +
             loc[2] * (dims[0]-1)*(dims[1]-1) +
             loc[1] * (dims[0]-1) + loc[0];
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkCell *vtkCTHData::FindAndGetCell(float x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      float vtkNotUsed(tol2), int& subId, 
                                      float pcoords[3], float *weights)
{
  int i, j, k, loc[3];
  vtkIdType npts, idx;
  int *dims = this->GetDimensions();
  vtkIdType d01 = dims[0]*dims[1];
  float xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  float *origin;
  float *spacing;
  int blockId, numBlocks;

  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = this->GetBlockOrigin(blockId);
    spacing = this->GetBlockSpacing(blockId);

    if ( this->ComputeStructuredCoordinates(blockId, x, loc, pcoords) != 0 )
      {
      //
      // Get the parametric coordinates and weights for interpolation
      //
      switch (this->DataDescription)
        {
        case VTK_EMPTY:
          return NULL;
        case VTK_SINGLE_POINT: // cellId can only be = 0
          vtkVertex::InterpolationFunctions(pcoords,weights);
          iMax = loc[0];
          jMax = loc[1];
          kMax = loc[2];
          cell = this->Vertex;
          break;
        case VTK_X_LINE:
          vtkLine::InterpolationFunctions(pcoords,weights);
          iMax = loc[0] + 1;
          jMax = loc[1];
          kMax = loc[2];
          cell = this->Line;
        break;
        case VTK_Y_LINE:
          vtkLine::InterpolationFunctions(pcoords,weights);
          iMax = loc[0];
          jMax = loc[1] + 1;
          kMax = loc[2];
          cell = this->Line;
          break;
        case VTK_Z_LINE:
          vtkLine::InterpolationFunctions(pcoords,weights);
          iMax = loc[0];
          jMax = loc[1];
          kMax = loc[2] + 1;
          cell = this->Line;
          break;
        case VTK_XY_PLANE:
          vtkPixel::InterpolationFunctions(pcoords,weights);
          iMax = loc[0] + 1;
          jMax = loc[1] + 1;
          kMax = loc[2];
          cell = this->Pixel;
          break;
        case VTK_YZ_PLANE:
          vtkPixel::InterpolationFunctions(pcoords,weights);
          iMax = loc[0];
          jMax = loc[1] + 1;
          kMax = loc[2] + 1;
          cell = this->Pixel;
          break;
        case VTK_XZ_PLANE:
          vtkPixel::InterpolationFunctions(pcoords,weights);
          iMax = loc[0] + 1;
          jMax = loc[1];
          kMax = loc[2] + 1;
          cell = this->Pixel;
          break;
        case VTK_XYZ_GRID:
          vtkVoxel::InterpolationFunctions(pcoords,weights);
          iMax = loc[0] + 1;
          jMax = loc[1] + 1;
          kMax = loc[2] + 1;
          cell = this->Voxel;
          break;
        }
      npts = 0;
      for (k = loc[2]; k <= kMax; k++)
        {
        xOut[2] = origin[2] + k * spacing[2]; 
        for (j = loc[1]; j <= jMax; j++)
          {
          xOut[1] = origin[1] + j * spacing[1]; 
          // make idx relative to the extent not the whole extent
          idx = loc[0]-this->Extent[0] + (j-this->Extent[2])*dims[0]
            + (k-this->Extent[4])*d01;
          for (i = loc[0]; i <= iMax; i++, idx++)
            {
            xOut[0] = origin[0] + i * spacing[0]; 

            cell->PointIds->SetId(npts,idx);
            cell->Points->SetPoint(npts++,xOut);
            }
          }
        }
      subId = 0;

      return cell;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkCTHData::GetCellType(vtkIdType vtkNotUsed(cellId))
{
  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      return VTK_EMPTY_CELL;

    case VTK_SINGLE_POINT: 
      return VTK_VERTEX;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE:
      return VTK_LINE;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE:
      return VTK_PIXEL;

    case VTK_XYZ_GRID:
      return VTK_VOXEL;

    default:
      vtkErrorMacro(<<"Bad data description!");
      return VTK_EMPTY_CELL;
    }
}

//----------------------------------------------------------------------------
void vtkCTHData::ComputeBounds()
{
  float bds[6];
  float *origin;
  float *spacing;
  int blockId, numBlocks;
  
  if ( this->Extent[0] > this->Extent[1] || 
       this->Extent[2] > this->Extent[3] ||
       this->Extent[4] > this->Extent[5] )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    return;
    }

  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = this->GetBlockOrigin(blockId);
    spacing = this->GetBlockSpacing(blockId);

    bds[0] = origin[0] + ((this->Extent[0]+this->NumberOfGhostLevels) * spacing[0]);
    bds[2] = origin[1] + ((this->Extent[2]+this->NumberOfGhostLevels) * spacing[1]);
    bds[4] = origin[2] + ((this->Extent[4]+this->NumberOfGhostLevels) * spacing[2]);

    bds[1] = origin[0] + ((this->Extent[1]-this->NumberOfGhostLevels) * spacing[0]);
    bds[3] = origin[1] + ((this->Extent[3]-this->NumberOfGhostLevels) * spacing[1]);
    bds[5] = origin[2] + ((this->Extent[5]-this->NumberOfGhostLevels) * spacing[2]);
    if (blockId == 0)
      {
      this->Bounds[0] = bds[0];
      this->Bounds[1] = bds[1];
      this->Bounds[2] = bds[2];
      this->Bounds[3] = bds[3];
      this->Bounds[4] = bds[4];
      this->Bounds[5] = bds[5];
      }
    else
      {
      if (bds[0] < this->Bounds[0]) {this->Bounds[0] = bds[0];}
      if (bds[1] > this->Bounds[1]) {this->Bounds[1] = bds[1];}
      if (bds[2] < this->Bounds[2]) {this->Bounds[2] = bds[2];}
      if (bds[3] > this->Bounds[3]) {this->Bounds[3] = bds[3];}
      if (bds[4] < this->Bounds[4]) {this->Bounds[4] = bds[4];}
      if (bds[5] > this->Bounds[5]) {this->Bounds[5] = bds[5];}
      }
    }
}



//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkCTHData::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkCTHData::SetDimensions(int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}


// streaming change: ijk is in extent coordinate system.
//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkCTHData::ComputeStructuredCoordinates(int blockId,
                                             float x[3], int ijk[3], 
                                             float pcoords[3])
{
  int i;
  float d, floatLoc;
  float *origin = this->GetBlockOrigin(blockId);
  float *spacing = this->GetBlockSpacing(blockId);
  int *dims = this->GetDimensions();
  
  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    floatLoc = d / spacing[i];
    // Floor for negtive indexes.
    ijk[i] = (int) (floor(floatLoc));
    if ( ijk[i] >= this->Extent[i*2] && ijk[i] < this->Extent[i*2 + 1] )
      {
      pcoords[i] = floatLoc - (float)ijk[i];
      }

    else if ( ijk[i] < this->Extent[i*2] || ijk[i] > this->Extent[i*2+1] ) 
      {
      return 0;
      } 

    else //if ( ijk[i] == this->Extent[i*2+1] )
      {
      if (dims[i] == 1)
        {
        pcoords[i] = 0.0;
        }
      else
        {
        ijk[i] -= 1;
        pcoords[i] = 1.0;
        }
      }

    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkCTHData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkIndent i2 = indent.GetNextIndent();
  int idx;
  int *dims = this->GetDimensions();
  int numBlocks = this->GetNumberOfBlocks();
  float* origin;
  float* spacing;
    
  os << indent << "Dimensions: (" << dims[0] << ", "
                                  << dims[1] << ", "
                                  << dims[2] << ")\n";
  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";
  os << indent << "NumberOfGhostLevels: " << this-> NumberOfGhostLevels << endl;

  os << indent << "NumberOfBlocks: "  << numBlocks << endl;
  for (idx = 0; idx < numBlocks; ++idx)
    {
    spacing = this->GetBlockSpacing(idx);
    origin = this->GetBlockOrigin(idx);
    os << i2 << "Origin: (" << origin[0] << ", "
                            << origin[1] << ", "
                            << origin[2] << "), ";
    os << "Spacing: (" << spacing[0] << ", "
                       << spacing[1] << ", "
                       << spacing[2] << ")\n";
    }
}



//----------------------------------------------------------------------------
void vtkCTHData::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
  this->SetExtent(ext);
}


//----------------------------------------------------------------------------
int *vtkCTHData::GetDimensions()
{
  this->Dimensions[0] = this->Extent[1] - this->Extent[0] + 1;
  this->Dimensions[1] = this->Extent[3] - this->Extent[2] + 1;
  this->Dimensions[2] = this->Extent[5] - this->Extent[4] + 1;

  return this->Dimensions;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetDimensions(int *dOut)
{
  int *dims = this->GetDimensions();
  dOut[0] = dims[0];
  dOut[1] = dims[1];
  dOut[2] = dims[2];  
}

//----------------------------------------------------------------------------
void vtkCTHData::SetExtent(int *extent)
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if ( description < 0 ) //improperly specified
    {
    vtkErrorMacro (<< "Bad Extent, retaining previous values");
    }
  
  if (description == VTK_UNCHANGED)
    {
    return;
    }

  this->DataDescription = description;
  
  this->Modified();
}


//----------------------------------------------------------------------------
unsigned long vtkCTHData::GetActualMemorySize()
{
  return this->vtkDataSet::GetActualMemorySize();
}


//----------------------------------------------------------------------------
void vtkCTHData::ShallowCopy(vtkDataObject *dataObject)
{
  vtkCTHData *cthData = vtkCTHData::SafeDownCast(dataObject);

  if ( cthData != NULL )
    {
    this->InternalCTHDataCopy(cthData);
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkCTHData::DeepCopy(vtkDataObject *dataObject)
{
  vtkCTHData *cthData = vtkCTHData::SafeDownCast(dataObject);

  if ( cthData != NULL )
    {
    this->InternalCTHDataCopy(cthData);
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkCTHData::InternalCTHDataCopy(vtkCTHData *src)
{
  int idx;

  this->SetNumberOfBlocks(src->GetNumberOfBlocks());
  for (int i = 0; i < src->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockOrigin(i, src->GetBlockOrigin(i));  
    this->SetBlockSpacing(i, src->GetBlockSpacing(i));  
    }

  this->DataDescription = src->DataDescription;
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    this->Extent[idx] = src->Extent[idx];
    this->Extent[idx+3] = src->Extent[idx+3];
    }
  this->NumberOfGhostLevels = src->NumberOfGhostLevels;
}



//----------------------------------------------------------------------------
vtkIdType vtkCTHData::GetNumberOfCellsPerBlock() 
{
  vtkIdType nCells=1;
  int i;
  int *dims = this->GetDimensions();

  for (i=0; i<3; i++)
    {
    if (dims[i] == 0)
      {
      return 0;
      }
    if (dims[i] > 1)
      {
      nCells *= (dims[i]-1);
      }
    }

  return nCells;
}

//----------------------------------------------------------------------------
vtkIdType vtkCTHData::GetNumberOfCells() 
{
  return this->GetNumberOfBlocks() * this->GetNumberOfCellsPerBlock();
}



//----------------------------------------------------------------------------
void vtkCTHData::SetUpdateExtent(int piece, int numPieces,
                                          int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
  this->UpdateExtentInitialized = 1;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetUpdateExtent(int &piece, int &numPieces,
                                          int &ghostLevel)
{
  piece = this->UpdatePiece;
  numPieces = this->UpdateNumberOfPieces;
  ghostLevel = this->UpdateGhostLevel;
}

