/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

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
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
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

vtkCxxRevisionMacro(vtkCTHData, "1.17");
vtkStandardNewMacro(vtkCTHData);

//----------------------------------------------------------------------------
vtkCTHData::vtkCTHData()
{
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();
  
  // This should be set up by user.
  this->DataDescription = VTK_EMPTY;
  
  this->BlockOrigins = vtkFloatArray::New();
  this->BlockOrigins->SetNumberOfComponents(3);
  this->BlockSpacings = vtkFloatArray::New();
  this->BlockSpacings->SetNumberOfComponents(3);
  this->BlockLevels = vtkIntArray::New();
  this->BlockLevels->SetNumberOfComponents(1);
  this->BlockCellExtents = vtkIntArray::New();
  this->BlockCellExtents->SetNumberOfComponents(6);

  this->TopLevelSpacing[0] = 1.0;
  this->TopLevelSpacing[1] = 1.0;
  this->TopLevelSpacing[2] = 1.0;

  this->TopLevelOrigin[0] = 0.0;
  this->TopLevelOrigin[1] = 0.0;
  this->TopLevelOrigin[2] = 0.0;

  this->NumberOfGhostLevels = 0;

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
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
  this->BlockLevels->Delete();
  this->BlockLevels = NULL;
  this->BlockCellExtents->Delete();
  this->BlockCellExtents = NULL;
}

//----------------------------------------------------------------------------
void vtkCTHData::Initialize()
{
  this->Superclass::Initialize();
  this->BlockOrigins->Initialize();
  this->BlockSpacings->Initialize();

  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  this->DataDescription = VTK_EMPTY;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetExtent(int extent[6])
{
  this->Information->Get(vtkDataObject::DATA_EXTENT(), extent);
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlock(int blockId, vtkImageData* block)
{
  double *tmp;
  int numCellsPerBlock = this->GetNumberOfCellsPerBlock();
  int numPointsPerBlock = this->GetNumberOfPointsPerBlock();

  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block id out of range.");
    return;
    }

  block->Initialize();
  block->SetDimensions(this->GetBlockPointDimensions(blockId));
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
  if (this->GetNumberOfBlocks() == num)
    {
    return;
    }
  
  this->Modified();
  this->BlockOrigins->SetNumberOfTuples(num);
  this->BlockSpacings->SetNumberOfTuples(num);
  this->BlockLevels->SetNumberOfTuples(num);
  this->BlockCellExtents->SetNumberOfTuples(num);

  memset(this->BlockOrigins->GetPointer(0), 0, num*3*sizeof(float));
  memset(this->BlockSpacings->GetPointer(0), 0, num*3*sizeof(float));
  memset(this->BlockLevels->GetPointer(0), 0, num*sizeof(int));
  memset(this->BlockCellExtents->GetPointer(0), 0, num*6*sizeof(int));
}

//----------------------------------------------------------------------------
int vtkCTHData::GetNumberOfBlocks()
{
  return this->BlockOrigins->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
int vtkCTHData::InsertNextBlock()
{
  double t[3];
  double e[6];
  t[0] = t[1] = t[2] = 0.0;
  this->BlockOrigins->InsertNextTuple(t);
  this->BlockSpacings->InsertNextTuple(t);
  this->BlockLevels->InsertNextValue(0);
  e[0] = e[1] = e[2] = e[3] = e[4] = e[5] = 0;
  this->BlockCellExtents->InsertNextTuple(e);
  return this->GetNumberOfBlocks()-1;
}


//----------------------------------------------------------------------------
void vtkCTHData::CellExtentToBounds(int level, int ext[6], double bds[6])
{
  int spacingFactor = 1;
  double spacing[3];
  spacingFactor = spacingFactor << level;
  
  spacing[0] = this->TopLevelSpacing[0] / (double)(spacingFactor);
  spacing[1] = this->TopLevelSpacing[1] / (double)(spacingFactor);
  spacing[2] = this->TopLevelSpacing[2] / (double)(spacingFactor);

  bds[0] = this->TopLevelOrigin[0] + (double)(ext[0]) * spacing[0];
  bds[1] = this->TopLevelOrigin[0] + (double)(ext[1]+1) * spacing[0];
  bds[2] = this->TopLevelOrigin[1] + (double)(ext[2]) * spacing[1];
  bds[3] = this->TopLevelOrigin[1] + (double)(ext[3]+1) * spacing[1];
  bds[4] = this->TopLevelOrigin[2] + (double)(ext[4]) * spacing[2];
  bds[5] = this->TopLevelOrigin[2] + (double)(ext[5]+1) * spacing[2];
}

void vtkCTHData::SetBlockCellExtent(int blockId, int level, int *extent)
{
  if (this->GetNumberOfBlocks() <= blockId)
    {
    vtkErrorMacro("Bad block id.");
    return;
    }

  int newDescription = VTK_XYZ_GRID;
  // We could do this with a case statement, but the index would be cryptic.
  int xtest = (extent[0] == extent[1]);
  int ytest = (extent[2] == extent[3]);
  int ztest = (extent[4] == extent[5]);
  if (xtest && ytest && ztest)
    {
    newDescription = VTK_SINGLE_POINT;
    }
  if (!xtest && ytest && ztest)
    {
    newDescription = VTK_X_LINE;
    }
  if (xtest && !ytest && ztest)
    {
    newDescription = VTK_Y_LINE;
    }
  if (xtest && ytest && !ztest)
    {
    newDescription = VTK_Z_LINE;
    }
  if (!xtest && !ytest && ztest)
    {
    newDescription = VTK_XY_PLANE;
    }
  if (!xtest && ytest && !ztest)
    {
    newDescription = VTK_XZ_PLANE;
    }
  if (xtest && !ytest && !ztest)
    {
    newDescription = VTK_YZ_PLANE;
    }

  if (this->DataDescription != VTK_EMPTY && this->DataDescription != newDescription)
    {
    vtkErrorMacro("Grids have mixed dimensionality.");
    }
  this->DataDescription = newDescription;

  this->BlockLevels->InsertValue(blockId, level);
  // Dumb to convert to double.
  double dumb[6];
  dumb[0] = extent[0];  dumb[1] = extent[1];  dumb[2] = extent[2];
  dumb[3] = extent[3];  dumb[4] = extent[4];  dumb[5] = extent[5];
  this->BlockCellExtents->InsertTuple(blockId, dumb);
  // Compute origin and spacing.
  double bds[6];
  double origin[3];
  double spacing[3];

  this->CellExtentToBounds(level, extent, bds);
  origin[0] = bds[0];
  origin[1] = bds[2];
  origin[2] = bds[4];
  spacing[0] = (bds[1]-bds[0])/(double)(extent[1]-extent[0]+1);
  spacing[1] = (bds[3]-bds[2])/(double)(extent[3]-extent[2]+1);
  spacing[2] = (bds[5]-bds[4])/(double)(extent[5]-extent[4]+1);

  this->BlockSpacings->InsertTuple(blockId, spacing);
  this->BlockOrigins->InsertTuple(blockId, origin);
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockCellExtent(int blockId, int level, 
                                    int e0, int e1, int e2, 
                                    int e3, int e4, int e5)
{
  int ext[6];
  ext[0] = e0;
  ext[1] = e1;
  ext[2] = e2;
  ext[3] = e3;
  ext[4] = e4;
  ext[5] = e5;
  this->SetBlockCellExtent(blockId, level, ext);
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockPointExtent(int blockId, int level, int *ext)
{
  this->SetBlockCellExtent(blockId, level, ext[0], ext[1]-1, 
                           ext[2], ext[3]-1, ext[4], ext[5]-1);
}

//----------------------------------------------------------------------------
void vtkCTHData::SetBlockPointExtent(int blockId, int level, 
                                     int e0, int e1, int e2, 
                                     int e3, int e4, int e5)
{
  this->SetBlockCellExtent(blockId, level, e0, e1-1, e2, e3-1, e4, e5-1);
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlockPointExtent(int blockId, int ext[6])
{
  this->GetBlockCellExtent(blockId, ext);
  // Handle Lower dimensions.  Assume that cell dimension of 1 is a collapsed
  // dimension.  Point dim equal 1 also.
  if (ext[1] > ext[0])
    {
    ++ext[1];
    }
  if (ext[3] > ext[2])
    {
    ++ext[3];
    }
  if (ext[5] > ext[4])
    {
    ++ext[5];
    }
}
//----------------------------------------------------------------------------
int* vtkCTHData::GetBlockPointExtent(int blockId)
{
  this->GetBlockPointExtent(blockId, this->TemporaryExtent);
  return this->TemporaryExtent;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlockPointDimensions(int blockId, int dim[3])
{
  int* ce = this->GetBlockCellExtent(blockId);
  if ( !ce )
    {
    vtkErrorMacro("Block Cell Extent not defined");
    return;
    }
  // Handle Lower dimensions.  Assume that cell dimension of 1 is a collapsed
  // dimension.  Point dim equal 1 also.
  dim[0] = dim[1] = dim[2] = 1;
  if (ce[1] > ce[0])
    {
    dim[0] = ce[1]-ce[0]+2;
    }
  if (ce[3] > ce[2])
    {
    dim[1] = ce[3]-ce[2]+2;
    }
  if (ce[5] > ce[4])
    {
    dim[2] = ce[5]-ce[4]+2;
    }
}
//----------------------------------------------------------------------------
int* vtkCTHData::GetBlockPointDimensions(int blockId)
{
  this->GetBlockPointDimensions(blockId, this->TemporaryDimensions);
  return this->TemporaryDimensions;
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlockCellExtent(int blockId, int ext[6])
{
  int* ce = this->GetBlockCellExtent(blockId);
  ext[0] = ce[0];
  ext[1] = ce[1];
  ext[2] = ce[2];
  ext[3] = ce[3];
  ext[4] = ce[4];
  ext[5] = ce[5];
}
//----------------------------------------------------------------------------
int* vtkCTHData::GetBlockCellExtent(int blockId)
{
  int *p = this->BlockCellExtents->GetPointer(0);
  return p+(blockId*6);
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlockCellDimensions(int blockId, int dim[3])
{
  int* ce = this->GetBlockCellExtent(blockId);
  dim[0] = ce[1]-ce[0]+1;
  dim[1] = ce[3]-ce[2]+1;
  dim[2] = ce[5]-ce[4]+1;
}
//----------------------------------------------------------------------------
int* vtkCTHData::GetBlockCellDimensions(int blockId)
{
  this->GetBlockCellDimensions(blockId, this->TemporaryDimensions);
  return this->TemporaryDimensions;
}



//----------------------------------------------------------------------------
double* vtkCTHData::GetBlockOrigin(int blockId)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return NULL;
    }
  return this->BlockOrigins->GetTuple(blockId);
}

//----------------------------------------------------------------------------
void vtkCTHData::GetBlockOrigin(int blockId, double origin[6])
{
  double *tmp = this->GetBlockOrigin(blockId);
  int i;
  for (i = 0; i < 3; ++i)
    {
    origin[i] = tmp[i];
    }
}

//----------------------------------------------------------------------------
double* vtkCTHData::GetBlockSpacing(int blockId)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return NULL;
    }
  return this->BlockSpacings->GetTuple(blockId);
}



//----------------------------------------------------------------------------
int vtkCTHData::GetBlockLevel(int blockId)
{
  if (blockId < 0 || blockId >= this->GetNumberOfBlocks())
    {
    vtkErrorMacro("Block out of range.");
    return 0;
    }
  return this->BlockLevels->GetValue(blockId);
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

  int *dims = this->GetBlockPointDimensions(blockId);

  vtkStructuredData::GetCellPoints(blockCellId,ptIds,this->DataDescription,
                                   dims);

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

  int* dims = this->GetBlockPointDimensions(blockId);
  
  vtkStructuredData::GetPointCells(ptId,cellIds,dims);
  ptr = cellIds->GetPointer(0);
  num = cellIds->GetNumberOfIds();
  for (id = 0; id < num; ++id)
    {
    *ptr += numCellsPerBlock * blockId;
    ++ptr;
    }
}



//----------------------------------------------------------------------------
void vtkCTHData::GetPoint(vtkIdType id, double x[3])
{
  double *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}



//----------------------------------------------------------------------------
// Get rid of this method ...
vtkIdType vtkCTHData::GetNumberOfPointsPerBlock()
{
  // hack
  int *dims = this->GetBlockPointDimensions(0);
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

  for (i = 0; i < 3; ++i)
    {
    this->TopLevelSpacing[i] = src->TopLevelSpacing[i];
    this->TopLevelOrigin[i] = src->TopLevelOrigin[i];
    }
  this->DataDescription = src->DataDescription;
  this->CopyInformation(src);

  this->SetNumberOfBlocks(src->GetNumberOfBlocks());
  for (i = 0; i < src->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockCellExtent(i, src->GetBlockLevel(i), src->GetBlockCellExtent(i));  
    }
}


//----------------------------------------------------------------------------
// The input data object must be of type vtkCTHData or a subclass!
void vtkCTHData::CopyTypeSpecificInformation( vtkDataObject *data )
{
  vtkCTHData *cth = vtkCTHData::SafeDownCast(data);
  int i;

  // Copy the generic stuff
  this->CopyInformation( data );
  
  if (cth == NULL)
    {
    return;
    }

  for (i = 0; i < 3; ++i)
    {
    this->TopLevelSpacing[i] = cth->TopLevelSpacing[i];
    this->TopLevelOrigin[i] = cth->TopLevelOrigin[i];
    }

  this->SetNumberOfBlocks(cth->GetNumberOfBlocks());
  for (i = 0; i < cth->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockCellExtent(i, cth->GetBlockLevel(i), 
                             cth->GetBlockCellExtent(i));  
    }
}


//----------------------------------------------------------------------------

vtkCell *vtkCTHData::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  // hack
  int *dims = this->GetBlockPointDimensions(0);
  int d01 = dims[0]*dims[1];
  double x[3];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  double *origin = this->GetBlockOrigin(blockId);
  double *spacing = this->GetBlockSpacing(blockId);

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
  npts = 0;
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2] * spacing[2]); 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1] * spacing[1]); 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0] * spacing[0]); 

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
  // hack
  int *dims = this->GetBlockPointDimensions(0);
  int d01 = dims[0]*dims[1];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockPointOffset;
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  double *origin = this->GetBlockOrigin(blockId);
  double *spacing = this->GetBlockSpacing(blockId);
  double x[3];

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
    x[2] = origin[2] + (loc[2] * spacing[2]); 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1] * spacing[1]); 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0] * spacing[0]); 

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
void vtkCTHData::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  int cellsPerBlock = this->GetNumberOfCellsPerBlock();
  int blockId = cellId / cellsPerBlock;
  int blockCellId = cellId - (blockId*cellsPerBlock);
  double *origin = this->GetBlockOrigin(blockId);
  double *spacing = this->GetBlockSpacing(blockId);
  // hack
  int *dims = this->GetBlockPointDimensions(0);

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


  bounds[0] = bounds[2] = bounds[4] =  VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;
  
  // Extract point coordinates
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2] * spacing[2]); 
    bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
    bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
    {
    x[1] = origin[1] + (loc[1] * spacing[1]); 
    bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
    bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
    }
  for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
    {
    x[0] = origin[0] + (loc[0] * spacing[0]); 
    bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
    bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
    }
}

//----------------------------------------------------------------------------
double *vtkCTHData::GetPoint(vtkIdType ptId)
{
  static double x[3];
  int i, loc[3];
  int pointsPerBlock = this->GetNumberOfPointsPerBlock();
  int blockId = ptId / pointsPerBlock;
  ptId = ptId - (blockId*pointsPerBlock);
  double *origin = this->GetBlockOrigin(blockId);
  double *spacing = this->GetBlockSpacing(blockId);

  if (spacing == 0)
    { // block out of range
    x[0] = x[1] = x[2] = 0.0;
    return x;
    }
   
  // hack
  int *dims = this->GetBlockPointDimensions(0);

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
    x[i] = origin[i] + (loc[i] * spacing[i]);
    }

  return x;
}

//----------------------------------------------------------------------------
// Find the closest point.
vtkIdType vtkCTHData::FindPoint(double x[3])
{
  int numPointsPerBlock = this->GetNumberOfPointsPerBlock();
  int blockId, numBlocks, bestBlock = 0, bestBlockPtId = 0;
  int i, loc[3];
  double d;
  double dist, bestDist;
  double *origin;
  double *spacing;
  int *dims;
  int extent[6];
  this->GetExtent(extent);

  // But force.
  // Loop through all blocks.
  bestDist = VTK_LARGE_FLOAT;
  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = this->GetBlockOrigin(blockId);
    spacing = this->GetBlockSpacing(blockId);
    dims = this->GetBlockPointDimensions(blockId);
    dist = 0.0;
    //
    //  Compute the ijk location
    //
    for (i=0; i<3; i++) 
      {
      d = x[i] - origin[i];
      loc[i] = (int) ((d / spacing[i]) + 0.5);
      if ( loc[i] < extent[i*2])
        {
        loc[i] = 0;
        }
      if (loc[i] >= dims[i])
        {
        loc[i] = dims[i]-1;
        }
      d = d - (spacing[i]*loc[i]);
      dist += d*d;
      // since point id is relative to the first point actually stored
      loc[i] -= 0;
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
vtkIdType vtkCTHData::FindCell(double x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId), 
                                  double vtkNotUsed(tol2), 
                                  int& subId, double pcoords[3], 
                                  double *weights)
{
  return
    this->FindCell( x, (vtkCell *)NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkCTHData::FindCell(double x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkIdType vtkNotUsed(cellId),
                                 double vtkNotUsed(tol2), 
                                 int& subId, double pcoords[3], double *weights)
{
  int loc[3];
  //hack
  int *dims = this->GetBlockPointDimensions(0);
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
vtkCell *vtkCTHData::FindAndGetCell(double x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      double vtkNotUsed(tol2), int& subId, 
                                      double pcoords[3], double *weights)
{
  int i, j, k, loc[3];
  vtkIdType npts, idx;
  // hack
  int *dims = this->GetBlockPointDimensions(0);
  vtkIdType d01 = dims[0]*dims[1];
  double xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  double *origin;
  double *spacing;
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
          idx = loc[0] + (j*dims[0])
            + (k*d01);
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
// We have to coimpute bounds around ghost cells, or locators in filters crash.
void vtkCTHData::ComputeBounds()
{
  double bds[6];
  double *origin;
  double *spacing;
  int *dims;
  int blockId, numBlocks;
  
  numBlocks = this->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = this->GetBlockOrigin(blockId);
    spacing = this->GetBlockSpacing(blockId);
    dims = this->GetBlockPointDimensions(blockId);

    bds[0] = origin[0];
    bds[2] = origin[1];
    bds[4] = origin[2];

    bds[1] = origin[0] + ((dims[0]-1) * spacing[0]);
    bds[3] = origin[1] + ((dims[1]-1) * spacing[1]);
    bds[5] = origin[2] + ((dims[2]-1) * spacing[2]);
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
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkCTHData::ComputeStructuredCoordinates(int blockId,
                                             double x[3], int ijk[3], 
                                             double pcoords[3])
{
  int i;
  double d, doubleLoc;
  double *origin = this->GetBlockOrigin(blockId);
  double *spacing = this->GetBlockSpacing(blockId);
  int *dims = this->GetBlockPointDimensions(blockId);
  
  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    doubleLoc = d / spacing[i];
    // Floor for negtive indexes.
    ijk[i] = (int) (floor(doubleLoc));
    if ( ijk[i] >= 0 && ijk[i] < dims[i]-1 )
      {
      pcoords[i] = doubleLoc - (double)ijk[i];
      }

    else if ( ijk[i] < 0 || ijk[i] >= dims[i] ) 
      {
      return 0;
      } 

    else //if ( ijk[i] == dims[i]-1 )
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
  // hack
  //int *dims = this->GetBlockPointDimensions(0);
  int numBlocks = this->GetNumberOfBlocks();
  double* origin;
  double* spacing;
  int *e;    

  os << indent << "TopLevelSpacing: (" << this->TopLevelSpacing[0] << ", "
                                  << this->TopLevelSpacing[1] << ", "
                                  << this->TopLevelSpacing[2] << ")\n";
  os << indent << "TopLevelOrigin: (" << this->TopLevelOrigin[0] << ", "
                                  << this->TopLevelOrigin[1] << ", "
                                  << this->TopLevelOrigin[2] << ")\n";

  os << indent << "NumberOfBlocks: "  << numBlocks << endl;
  for (idx = 0; idx < numBlocks; ++idx)
    {  
    spacing = this->GetBlockSpacing(idx);
    origin = this->GetBlockOrigin(idx);
    e = this->GetBlockCellExtent(idx);
    os << i2 << "Level: " << this->GetBlockLevel(idx) << " ";
    os << i2 << "\tOrigin: (" << origin[0] << ", "
                            << origin[1] << ", "
                            << origin[2] << "), ";
    os << "\tSpacing: (" << spacing[0] << ", "
                       << spacing[1] << ", "
                       << spacing[2] << ")\n";
    os << "\tCellExt: (" << e[0] << ", " << e[1] << ", " << e[2]
       << e[3] << ", " << e[4] << ", " << e[5] << ")\n";
    }
  os << indent << "NumberOfGhostLevels: " << this->NumberOfGhostLevels << endl;
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

  this->DataDescription = src->DataDescription;
  for (idx = 0; idx < 3; ++idx)
    {
    this->TopLevelOrigin[idx] = src->TopLevelOrigin[idx];
    this->TopLevelSpacing[idx] = src->TopLevelSpacing[idx];
    }

  this->NumberOfGhostLevels = src->NumberOfGhostLevels;

  this->SetNumberOfBlocks(src->GetNumberOfBlocks());
  for (int i = 0; i < src->GetNumberOfBlocks(); ++i)
    {
    this->SetBlockCellExtent(i, src->GetBlockLevel(i), 
                             src->GetBlockCellExtent(i));  
    }
}



//----------------------------------------------------------------------------
vtkIdType vtkCTHData::GetNumberOfCellsPerBlock() 
{
  vtkIdType nCells=1;
  int i;
  // hack
  int *dims = this->GetBlockPointDimensions(0);

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
  this->SetUpdatePiece(piece);
  this->SetUpdateNumberOfPieces(numPieces);
  this->SetUpdateGhostLevel(ghostLevel);
}

//----------------------------------------------------------------------------
void vtkCTHData::GetUpdateExtent(int &piece, int &numPieces,
                                          int &ghostLevel)
{
  piece = this->GetUpdatePiece();
  numPieces = this->GetUpdateNumberOfPieces();
  ghostLevel = this->GetUpdateGhostLevel();
}

