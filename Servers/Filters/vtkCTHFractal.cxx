/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHFractal.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

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
#include "vtkDoubleArray.h"
#include "vtkImageMandelbrotSource.h"



vtkCxxRevisionMacro(vtkCTHFractal, "1.12");
vtkStandardNewMacro(vtkCTHFractal);

//----------------------------------------------------------------------------
vtkCTHFractal::vtkCTHFractal()
{
  this->Dimensions = 10;
  this->FractalValue = 9.5;
  this->MaximumLevel = 6;
  this->GhostLevels = 0;

  this->Levels = vtkIntArray::New();
  this->TwoDimensional = 1;
}

//----------------------------------------------------------------------------
vtkCTHFractal::~vtkCTHFractal()
{
  this->Levels->Delete();
  this->Levels = NULL;
}

//----------------------------------------------------------------------------
// This handles any alterations necessary for ghost levels.
void vtkCTHFractal::SetBlockInfo(int blockId, int level, 
                                int x0, int y0, int z0)
{
  int ext[6];
  
  ext[0] = x0;
  ext[2] = y0;
  ext[4] = z0;
  ext[1] = x0+this->Dimensions-1;
  ext[3] = y0+this->Dimensions-1;
  ext[5] = z0+this->Dimensions-1;
  
  if (this->GhostLevels)
    {
    ext[0] -= 1;
    ext[2] -= 1;
    ext[4] -= 1;
    ext[1] += 1;
    ext[3] += 1;
    ext[5] += 1;
    }
  if (this->TwoDimensional)
    {
    ext[4] = ext[5] = 0;
    }
  this->GetOutput()->SetBlockCellExtent(blockId, level,
                             ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
}

//----------------------------------------------------------------------------
int vtkCTHFractal::TwoDTest(double bds[6], int level, int target) 
{
  // Test the 4 corners.  Refine if the blocks cross the border.
  int v0, v1, v2, v3;
  
  if (level == target)
    {
    return 0;
    }
  
  if (level < 2)
    {
    return 1;
    }
  
  v0 = this->MandelbrotTest(bds[0], bds[2]);
  v1 = this->MandelbrotTest(bds[1], bds[2]);
  v2 = this->MandelbrotTest(bds[0], bds[3]);
  v3 = this->MandelbrotTest(bds[1], bds[3]);
  if (v0 && v1 && v2 && v3)
    {
    return 0;
    }
  if (!v0 && !v1 && !v2 && !v3)
    {
    return 0;
    }
  return 1;
}

int vtkCTHFractal::MandelbrotTest(double x, double y)
{
  unsigned short count = 0;
  double v0, v1;
  double cReal, cImag, zReal, zImag;
  double zReal2, zImag2;

  cReal = x;
  cImag = y;
  zReal = 0.0;
  zImag = 0.0;

  zReal2 = zReal * zReal;
  zImag2 = zImag * zImag;
  v0 = 0.0;
  v1 = (zReal2 + zImag2);
  while ( v1 < 4.0 && count < 100)
    {
    zImag = 2.0 * zReal * zImag + cImag;
    zReal = zReal2 - zImag2 + cReal;
    zReal2 = zReal * zReal;
    zImag2 = zImag * zImag;
    ++count;
    v0 = v1;
    v1 = (zReal2 + zImag2);
    }

  if (count == 100)
    {
    return 1;
    }

  return 0;
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
  output->SetTopLevelOrigin(ox, oy, oz);
  output->SetTopLevelSpacing(xSize/this->Dimensions,
                             ySize/this->Dimensions,
                             zSize/this->Dimensions);
  if (this->GhostLevels)
    {
    output->SetNumberOfGhostLevels(1);
    }
  else
    {
    output->SetNumberOfGhostLevels(0);
    }

  this->Levels->Initialize();
  this->Traverse(blockId, 0, output, 0, 0, 0);

  this->AddTestArray();
  this->AddFractalArray();
  this->AddBlockIdArray();
  this->AddDepthArray();

  if (this->GhostLevels > 0)
    {
    this->AddGhostLevelArray();
    }
}
  

int vtkCTHFractal::LineTest2(float x0, float y0, float z0,
                             float x1, float y1, float z1, 
                             double bds[6]) 
{
  // intersect line with plane.
  float x, y, z;
  float k;

  // Special case ane point is inside box.
  if (x0>bds[0] && x0<bds[1] && y0>bds[2] && y0<bds[3] && z0>bds[4] && z0<bds[5])
    {
    return 1;
    }
  if (x1>bds[0] && x1<bds[1] && y1>bds[2] && y1<bds[3] && z1>bds[4] && z1<bds[5])
    {
    return 1;
    }

  // Do not worry about divide by zero.
  // min x
  x = bds[0];
  k = (x- x0) / (x1-x0);
  if (k >=0.0 && k <= 1.0)
    {
    y = y0 + k*(y1-y0);
    z = z0 + k*(z1-z0);
    if (y >= bds[2] && y <= bds[3] && z >= bds[4] && z <= bds[5])
      {
      return 1;
      }
    } 
  // max x
  x = bds[1];
  k = (x- x0) / (x1-x0);
  if (k >=0.0 && k <= 1.0)
    {
    y = y0 + k*(y1-y0);
    z = z0 + k*(z1-z0);
    if (y >= bds[2] && y <= bds[3] && z >= bds[4] && z <= bds[5])
      {
      return 1;
      }
    } 
  // min y
  y = bds[2];
  k = (y- y0) / (y1-y0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    z = z0 + k*(z1-z0);
    if (x >= bds[0] && x <= bds[1] && z >= bds[4] && z <= bds[5])
      {
      return 1;
      }
    } 
  // max y
  y = bds[3];
  k = (y- y0) / (y1-y0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    z = z0 + k*(z1-z0);
    if (x >= bds[0] && x <= bds[1] && z >= bds[4] && z <= bds[5])
      {
      return 1;
      }
    } 
  // min z
  z = bds[4];
  k = (z- z0) / (z1-z0);
  if (k >=0.0 && k <= 1.0)
    {
    x = x0 + k*(x1-x0);
    y = y0 + k*(y1-y0);
    if (y >= bds[2] && y <= bds[3] && x >= bds[0] && x <= bds[1])
      {
      return 1;
      }
    } 

  return 0;
}

int vtkCTHFractal::LineTest(float x0, float y0, float z0, 
                            float x1, float y1, float z1,
                            double bds[6], int level, int target) 
{
  if (level >= target)
    {
    return 0;
    }
  // First check to see if the line intersects this block.
  if (this->LineTest2(x0, y0, z0, x1, y1, z1, bds))
    {
    return 1;
    }

  // If the line intersects our neighbor, then our levels cannot differ by more than one.
  // Assume that our neighbor is half our size.
  double bds2[6];
  memcpy(bds2, bds, 6*sizeof(double));
  target = target - 1;
  float size;

  size = 0.5*(bds[1]-bds[0]);
  bds2[0] = bds[0] - size;
  bds2[1] = bds[1] + size;
  if (this->LineTest(x0, y0, z0, x1, y1, z1, bds2, level, target))
    {
    return 1;
    }
  bds2[0] = bds[0];
  bds2[1] = bds[1];

  size = 0.5*(bds[3]-bds[2]);
  bds2[2] = bds[2] - size;
  bds2[3] = bds[3] + size;
  if (this->LineTest(x0, y0, z0, x1, y1, z1, bds2, level, target))
    {
    return 1;
    }
  bds2[2] = bds[2];
  bds2[3] = bds[3];

  size = 0.5*(bds[5]-bds[4]);
  bds2[4] = bds[4] - size;
  bds2[5] = bds[5] + size;
  if (this->LineTest(x0, y0, z0, x1, y1, z1, bds2, level, target))
    {
    return 1;
    }

  return 0;
}



//----------------------------------------------------------------------------
void vtkCTHFractal::Traverse(int &blockId, int level, vtkCTHData* output, 
                             int x0, int y0, int z0)
{
  int ext[6];
  double bds[6];
  int dim = this->Dimensions;

  // Compute cell extent.
  ext[0] = x0;
  ext[1] = x0+dim-1;
  ext[2] = y0;
  ext[3] = y0+dim-1;
  ext[4] = z0;
  ext[5] = z0+dim-1;
  
  if (this->TwoDimensional)
    {
    ext[4] = ext[5] = 0;
    }  
  
  // Get the bounds of the proposed block.
  output->CellExtentToBounds(level, ext, bds);

  if (this->TwoDimensional)
    {
    if (this->TwoDTest(bds, level, this->MaximumLevel))
      {
      ++level;
      x0 = 2 * x0;
      y0 = 2 * y0;
      // Traverse the 4 new blocks.
      this->Traverse(blockId, level, output, x0, y0, z0);
      this->Traverse(blockId, level, output, x0+dim, y0, z0);
      this->Traverse(blockId, level, output, x0, y0+dim, z0);
      this->Traverse(blockId, level, output, x0+dim, y0+dim, z0);
      }
    else
      {
      if (output->InsertNextBlock() != blockId)
        {
        vtkErrorMacro("blockId wrong.")
        return;
        }
      this->Levels->InsertValue(blockId, level);
      this->SetBlockInfo(blockId++, level, x0, y0, z0);
      }
    }
  else
    { // 3D
    if (this->LineTest(-1.64662,0.56383,1.16369, -1.05088,0.85595,0.87104, bds, level, this->MaximumLevel) ||
        this->LineTest(-1.05088,0.85595,0.87104, -0.61430,1.00347,0.59553, bds, level, this->MaximumLevel) )
      { // break block into eight.
      ++level;
      x0 = 2 * x0;
      y0 = 2 * y0;
      z0 = 2 * z0;
      // Traverse the 8 new blocks.
      this->Traverse(blockId, level, output, x0, y0, z0);
      this->Traverse(blockId, level, output, x0+dim, y0, z0);
      this->Traverse(blockId, level, output, x0, y0+dim, z0);
      this->Traverse(blockId, level, output, x0+dim, y0+dim, z0);
      this->Traverse(blockId, level, output, x0, y0, z0+dim);
      this->Traverse(blockId, level, output, x0+dim, y0, z0+dim);
      this->Traverse(blockId, level, output, x0, y0+dim, z0+dim);
      this->Traverse(blockId, level, output, x0+dim, y0+dim, z0+dim);
      }
    else
      {
      if (output->InsertNextBlock() != blockId)
        {
        vtkErrorMacro("blockId wrong.")
        return;
        }
      this->Levels->InsertValue(blockId, level);
      this->SetBlockInfo(blockId++, level, x0, y0, z0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkCTHFractal::AddTestArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkDoubleArray* array = vtkDoubleArray::New();
  int blockId;
  double* arrayPtr;
  double* spacing;
  double* origin;

  array->Allocate(numCells);
  array->SetNumberOfTuples(numCells);
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    array->Delete();
    return;
    }
  arrayPtr = (double*)(array->GetPointer(0));

  origin = output->GetTopLevelOrigin();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    spacing = output->GetBlockSpacing(blockId);
    int x,y,z;
    int ext[6];
    output->GetBlockCellExtent(blockId,ext);
    for (z = ext[4]; z <= ext[5]; ++z)
      {
      for (y = ext[2]; y <= ext[3]; ++y)
        {
        for (x = ext[0]; x <= ext[1]; ++x)
          {
          *arrayPtr++ = origin[0] + spacing[0]*((float)x + 0.5)
                        + origin[1] + spacing[1]*((float)y + 0.5);
//          *arrayPtr++ = origin[1] + spacing[1]*((float)y + 0.5);
//                        + origin[2] + spacing[2]*((float)z + 0.5);
          }
        }
      }
    }
  
  array->SetName("TestX");
  output->GetCellData()->AddArray(array);
  array->Delete();
}

//----------------------------------------------------------------------------
void vtkCTHFractal::AddFractalArray()
{
  vtkCTHData* output = this->GetOutput();
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  int numCellsPerBlock = output->GetNumberOfCellsPerBlock();
  vtkDoubleArray* array = vtkDoubleArray::New();
  int blockId;
  double* arrayPtr;
  vtkDataArray* fractal;
  float* fractalPtr;
  vtkImageMandelbrotSource* fractalSource = vtkImageMandelbrotSource::New();
  double* spacing;
  double* origin;
  int dims[3];

  array->Allocate(numCells);
  array->SetNumberOfTuples(numCells);
  if (numCells != numBlocks*numCellsPerBlock)
    {
    vtkErrorMacro("Cell count error.");
    array->Delete();
    return;
    }
  arrayPtr = (double*)(array->GetPointer(0));

  // hack
  output->GetBlockPointDimensions(0, dims);
  if (this->TwoDimensional)
    {
    dims[2] = 2;
    }
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
  double *spacing;
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
  int dims[3];
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
    output->GetBlockCellDimensions(blockId, dims);    
    for (k = 0; k < dims[2]; ++k)
      {
      kLevel = this->GhostLevels - k;
      tmp = k - dims[2] + 1 + this->GhostLevels;
      if (tmp > kLevel) { kLevel = tmp;}
      if (this->TwoDimensional)
        {
        kLevel = 0;
        }
      for (j = 0; j < dims[1]; ++j)
        {
        jLevel = kLevel;
        tmp = this->GhostLevels - j;
        if (tmp > jLevel) { jLevel = tmp;}
        tmp = j - dims[1] + 1 + this->GhostLevels;
        if (tmp > jLevel) { jLevel = tmp;}
        for (i = 0; i < dims[0]; ++i)
          {
          iLevel = jLevel;
          tmp = this->GhostLevels - i;
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
  os << indent << "TwoDimensional: " << this->TwoDimensional << endl;
  os << indent << "FractalValue: " << this->FractalValue << endl;
  os << indent << "MaximumLevel: " << this->MaximumLevel << endl;
  os << indent << "GhostLevels: " << this->GhostLevels << endl;
}

