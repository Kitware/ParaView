/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFractal.cxx
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
#include "vtkCTHFractal.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkImageMandelbrotSource.h"



vtkCxxRevisionMacro(vtkCTHFractal, "1.3");
vtkStandardNewMacro(vtkCTHFractal);

//----------------------------------------------------------------------------
vtkCTHFractal::vtkCTHFractal()
{
  this->Dimensions = 10;
  this->FractalValue = 9.5;
  this->MaximumLevel = 6;
  this->GhostLevels = 0;

  this->Levels = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkCTHFractal::~vtkCTHFractal()
{
  this->Levels->Delete();
  this->Levels = NULL;
}

//----------------------------------------------------------------------------
void vtkCTHFractal::SetDimensions(int xDim, int yDim, int zDim)
{
  this->GetOutput()->SetDimensions(xDim + 1 + (2*this->GhostLevels),
                                   yDim + 1 + (2*this->GhostLevels),
                                   zDim + 1 + (2*this->GhostLevels));
  this->GetOutput()->SetNumberOfGhostLevels(this->GhostLevels);
}

//----------------------------------------------------------------------------
// This handles any alterations necessary for ghost levels.
void vtkCTHFractal::SetBlockInfo(int blockId, 
                                 float ox, float oy, float oz,
                                 float xSize, float ySize, float zSize)
{
  // Compute spacing;
  int dims[3];
  this->GetOutput()->GetDimensions(dims);
  float sx, sy, sz;
  
  // Cell dimensions of block with no ghost levels.
  // Wee need to compute this to get spacing.
  dims[0] -= 1+ 2*this->GhostLevels;
  dims[1] -= 1+ 2*this->GhostLevels;
  dims[2] -= 1+ 2*this->GhostLevels;

  sx = xSize / dims[0];
  sy = ySize / dims[1];
  sz = zSize / dims[2];

  ox -= sx * this->GhostLevels;
  oy -= sy * this->GhostLevels;
  oz -= sz * this->GhostLevels;
  this->GetOutput()->SetBlockOrigin(blockId, ox, oy, oz);
  this->GetOutput()->SetBlockSpacing(blockId, sx, sy, sz);
}

//----------------------------------------------------------------------------
void vtkCTHFractal::Execute()
{
  vtkCTHData* output = this->GetOutput();
  float ox = -1.75;
  float oy = -1.25;
  float oz = 0.0;
  float xSize = 2.5;
  float ySize = 2.5;
  float zSize = 2.0;
  int blockId = 0;

  // This is 10x10x10 in cells.
  this->SetDimensions(this->Dimensions, this->Dimensions, this->Dimensions);
  this->Levels->Initialize();
  this->Traverse(blockId, 0, output, ox, oy, oz, xSize, ySize, zSize);

  this->AddFractalArray();
  this->AddBlockIdArray();
  this->AddDepthArray();

  if (this->GhostLevels > 0)
    {
    this->AddGhostLevelArray();
    }
}
  

//----------------------------------------------------------------------------
#define cthTest(x,y,z,l) \
  (x>ox1)&&(x<ox1+sx1)&&(y>oy1)&&(y<oy1+sy1)&&(z>oz1)&&(z<oz1+sz1)&&(level<l) 



int vtkCTHFractal::LineTest2(float x0, float y0, float z0, 
                             float x1, float y1, float z1,
                             float ox, float oy, float oz,
                             float sx, float sy, float sz,
                             int level, int target) 
{
  float mx = ox + sx;
  float my = oy + sy;
  float mz = oz + sz;
  // intersect line with plane.
  float x, y, z;
  float k;

  // Special case ane point is inside box.
  if (x0 > ox && x0 < mx && y0 > oy && y0 < my && z0 > oz && z0 < mz)
    {
    return (level < target);
    }
  if (x1 > ox && x1 < mx && y1 > oy && y1 < my && z1 > oz && z1 < mz)
    {
    return (level < target);
    }

  // Do not worry about divide by zero.
  // min x
  x = ox;
  k = (x- x0) / (x1-x0);
  if (k >=0.0 && k <= 1.0)
    {
    y = y0 + k*(y1-y0);
    z = z0 + k*(z1-z0);
    if (y >= oy && y <= my && z >= oz && z <= mz)
      {
      return (level < target);
      }
    } 
  // max x
  x = mx;
  k = (x- x0) / (x1-x0);
  if (k >=0.0 && k <= 1.0)
    {
    y = y0 + k*(y1-y0);
    z = z0 + k*(z1-z0);
    if (y >= oy && y <= my && z >= oz && z <= mz)
      {
      return (level < target);
      }
    } 
  // min y
  y = oy;
  k = (y- y0) / (y1-y0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    z = z0 + k*(z1-z0);
    if (x >= ox && x <= mx && z >= oz && z <= mz)
      {
      return (level < target);
      }
    } 
  // max y
  y = my;
  k = (y- y0) / (y1-y0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    z = z0 + k*(z1-z0);
    if (x >= ox && x <= mx && z >= oz && z <= mz)
      {
      return (level < target);
      }
    } 
  // min z
  z = oz;
  k = (z- z0) / (z1-z0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    y = y0 + k*(y1-y0);
    if (y >= oy && y <= my && x >= ox && x <= mx)
      {
      return (level < target);
      }
    } 

  return 0;
}

int vtkCTHFractal::LineTest(float x0, float y0, float z0, 
                            float x1, float y1, float z1,
                            float ox, float oy, float oz,
                            float sx, float sy, float sz,
                            int level, int target) 
{
  // First check to see if the line intersects this block.
  if (this->LineTest2(x0, y0, z0, x1, y1, z1, ox, oy, oz, sx, sy, sz, level, target))
    {
    return 1;
    }

  // If the line intersects our neighbor, then our levels cannot differ by mopre than one.
  // Assume that our neighbor is half our size.
  target = target - 1;
  ox = ox - 0.5*sx;
  oy = oy - 0.5*sy;
  oz = oz - 0.5*sz;
  sx = sx * 2.0;
  sy = sy * 2.0;
  sz = sz * 2.0;

  if (this->LineTest2(x0, y0, z0, x1, y1, z1, ox, oy, oz, sx, sy, sz, level, target))
    {
    return 1;
    }
  return 0;
}



//----------------------------------------------------------------------------
void vtkCTHFractal::Traverse(int &blockId, int level, vtkCTHData* output, 
                             float ox1, float oy1, float oz1,
                             float sx1, float sy1, float sz1)
{
  if (this->LineTest(-1.64662,0.56383,1.16369, -1.05088,0.85595,0.87104, ox1,oy1,oz1, sx1,sy1,sz1, level, this->MaximumLevel) ||
      this->LineTest(-1.05088,0.85595,0.87104, -0.61430,1.00347,0.59553, ox1,oy1,oz1, sx1,sy1,sz1, level, this->MaximumLevel) )
    { // break block into eight.
    ++level;
    float mx1, my1, mz1;
    float sx2, sy2, sz2;
    int* dims = output->GetDimensions();
    // Compute the midpoint of old block.
    mx1 = ox1 + 0.5 * sx1;
    my1 = oy1 + 0.5 * sy1;
    mz1 = oz1 + 0.5 * sz1;
    // Compute the size of the new blocks.
    sx2 = sx1 * 0.5;
    sy2 = sy1 * 0.5;
    sz2 = sz1 * 0.5;
    // Traverse the 8 new blocks.
    this->Traverse(blockId, level, output, ox1, oy1, oz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, mx1, oy1, oz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, ox1, my1, oz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, mx1, my1, oz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, ox1, oy1, mz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, mx1, oy1, mz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, ox1, my1, mz1, sx2, sy2, sz2);
    this->Traverse(blockId, level, output, mx1, my1, mz1, sx2, sy2, sz2);
    }
  else
    {
    if (output->InsertNextBlock() != blockId)
      {
      vtkErrorMacro("blockId wrong.")
      return;
      }
    this->Levels->InsertValue(blockId, level);
    this->SetBlockInfo(blockId++, ox1, oy1, oz1, sx1, sy1, sz1);
    }
}


//----------------------------------------------------------------------------
void vtkCTHFractal::AddFractalArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkFloatArray* array = vtkFloatArray::New();
  int blockId;
  float* arrayPtr;
  vtkDataArray* fractal;
  float* fractalPtr;
  vtkImageMandelbrotSource* fractalSource = vtkImageMandelbrotSource::New();
  float* spacing;
  float* origin;
  int dims[3];

  array->Allocate(numCells);
  array->SetNumberOfTuples(numCells);
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    array->Delete();
    return;
    }
  arrayPtr = (float*)(array->GetPointer(0));

  output->GetDimensions(dims);
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    origin = output->GetBlockOrigin(blockId);
    spacing = output->GetBlockSpacing(blockId);
    // Shift point to center of voxel.
    fractalSource->SetWholeExtent(0,dims[0]-2, 0,dims[1]-2, 0,dims[2]-2);
    fractalSource->SetOriginCX(origin[0]+(spacing[0]*0.5), 
                               origin[1]+(spacing[1]*0.5), 
                               origin[2]+(spacing[2]*0.5), 0.0);
    fractalSource->SetSampleCX(spacing[0], spacing[1], spacing[2], 0.1);
    fractalSource->Update();
    fractal = fractalSource->GetOutput()->GetPointData()->GetScalars();
    if (fractal->GetNumberOfTuples() != numCellsPerBlock)
      {
      vtkErrorMacro("point to cell mismatch.");
      }
    fractalPtr = (float*)(fractal->GetVoidPointer(0));

    //memcpy(arrayPtr, fractalPtr, sizeof(float)*numCellsPerBlock);
    //arrayPtr += numCellsPerBlock;
    for (int i = 0; i < numCellsPerBlock; ++i)
      {
      // Change fractal into volume fraction (iso surface at 0.5).
      *arrayPtr++ = *fractalPtr++ / (2.0 * this->FractalValue);
      }
    }
  
  array->SetName("Fractal Volume Fraction");
  output->GetCellData()->AddArray(array);
  array->Delete();
  fractalSource->Delete();
}


//----------------------------------------------------------------------------
void vtkCTHFractal::AddBlockIdArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkIntArray* blockArray = vtkIntArray::New();
  int blockId;
  int blockCellId;

  blockArray->Allocate(numCells);
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    blockArray->Delete();
    return;
    }

  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    for (blockCellId = 0; blockCellId < numCellsPerBlock; ++blockCellId)
      {
      blockArray->InsertNextValue(blockId);
      }
    }
  
  blockArray->SetName("BlockId");
  output->GetCellData()->AddArray(blockArray);
  blockArray->Delete();
}


//----------------------------------------------------------------------------
void vtkCTHFractal::AddDepthArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkIntArray* depthArray = vtkIntArray::New();
  int blockId;
  int blockCellId;
  float *spacing;
  int depth;

  depthArray->Allocate(numCells);
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    depthArray->Delete();
    return;
    }

  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    spacing = output->GetBlockSpacing(blockId);
    depth = this->Levels->GetValue(blockId);
    for (blockCellId = 0; blockCellId < numCellsPerBlock; ++blockCellId)
      {
      depthArray->InsertNextValue(depth);
      }
    }
  
  depthArray->SetName("Depth");
  output->GetCellData()->AddArray(depthArray);
  depthArray->Delete();
}

//----------------------------------------------------------------------------
void vtkCTHFractal::AddGhostLevelArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkUnsignedCharArray* array = vtkUnsignedCharArray::New();
  int blockId;
  int *dims = output->GetDimensions();
  int i, j, k;
  unsigned char* ptr;
  int iLevel, jLevel, kLevel, tmp;

  array->SetNumberOfTuples(numCells);
  ptr = (unsigned char*)(array->GetVoidPointer(0));
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    array->Delete();
    return;
    }

  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    for (k = 1; k < dims[2]; ++k)
      {
      kLevel = this->GhostLevels - k + 1;
      tmp = k - dims[2] + 1 + this->GhostLevels;
      if (tmp > kLevel) { kLevel = tmp;}
      for (j = 1; j < dims[1]; ++j)
        {
        jLevel = kLevel;
        tmp = this->GhostLevels - j + 1;
        if (tmp > jLevel) { jLevel = tmp;}
        tmp = j - dims[1] + 1 + this->GhostLevels;
        if (tmp > jLevel) { jLevel = tmp;}
        for (i = 1; i < dims[0]; ++i)
          {
          iLevel = jLevel;
          tmp = this->GhostLevels - i + 1;
          if (tmp > iLevel) { iLevel = tmp;}
          tmp = i - dims[0] + 1 + this->GhostLevels;
          if (tmp > iLevel) { iLevel = tmp;}

          if (iLevel <= 0)
            {
            *ptr = 0;
            }
          else
            {
            *ptr = iLevel;
            }
          ++ptr;
          }
        }
      }
    }

  //array->SetName("Test");
  array->SetName("vtkGhostLevels");
  output->GetCellData()->AddArray(array);
  array->Delete();
}


//----------------------------------------------------------------------------
void vtkCTHFractal::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensions: " << this->Dimensions << endl;
  os << indent << "FractalValue: " << this->FractalValue << endl;
  os << indent << "MaximumLevel: " << this->MaximumLevel << endl;


}

