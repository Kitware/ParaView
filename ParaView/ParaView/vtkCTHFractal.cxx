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



vtkCxxRevisionMacro(vtkCTHFractal, "1.2");
vtkStandardNewMacro(vtkCTHFractal);

//----------------------------------------------------------------------------
vtkCTHFractal::vtkCTHFractal()
{
  this->Dimensions = 10;
  this->FractalValue = 8.5;
  this->GhostLevels = 1;
}

//----------------------------------------------------------------------------
vtkCTHFractal::~vtkCTHFractal()
{
}

//----------------------------------------------------------------------------
void vtkCTHFractal::SetDimensions(int xDim, int yDim, int zDim)
{
  this->GetOutput()->SetDimensions(xDim + (2*this->GhostLevels),
                                   yDim + (2*this->GhostLevels),
                                   zDim + (2*this->GhostLevels));
  this->GetOutput()->SetNumberOfGhostLevels(this->GhostLevels);
}

//----------------------------------------------------------------------------
void vtkCTHFractal::SetBlockInfo(int blockId, 
                                 float ox, float oy, float oz,
                                 float sx, float sy, float sz)
{
  ox -= sx * this->GhostLevels;
  oy -= sy * this->GhostLevels;
  oz -= sz * this->GhostLevels;
  this->GetOutput()->SetBlockOrigin(blockId, ox, oy, oz);
  this->GetOutput()->SetBlockSpacing(blockId, sx, sy, sz);
}

/*
//----------------------------------------------------------------------------
void vtkCTHFractal::Execute()
{
  int blockId = 0;
  vtkCTHData* output = this->GetOutput();

  // This is 10x10x10 in cells.
  int dim = this->Dimensions;
  float xOrigin = -1.75;
  float yOrigin = -1.25;
  float zOrigin = 0.0;
  float xSize = 2.5;
  float ySize = 2.5;
  float zSize = 2.0;

  float ox1, oy1, oz1;
  float sx1, sy1, sz1;
  float mx1, my1, mz1;


  this->SetDimensions(dim+1, dim+1, dim+1);
  // Each refinement adds seven (adds eight takes away 1).
  output->SetNumberOfBlocks(8);

  // Lowest resolution 8
  sx1 = xSize / (2.0 * dim);
  sy1 = ySize / (2.0 * dim);
  sz1 = zSize / (2.0 * dim);
  ox1 = xOrigin;
  oy1 = yOrigin;
  oz1 = zOrigin;

  mx1 = ox1 + xSize / 2.0;
  my1 = oy1 + ySize / 2.0;
  mz1 = oz1 + zSize / 2.0;
  
  this->SetBlockInfo(blockId++, ox1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, oy1, mz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, mz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, my1, mz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, my1, mz1, sx1, sy1, sz1);

  this->AddFractalArray();
  this->AddBlockIdArray();
  this->AddDepthArray(sx1);

  if (this->GhostLevels > 0)
    {
    this->AddGhostLevelArray();
    }
}
*/

/*
//----------------------------------------------------------------------------
void vtkCTHFractal::Execute()
{
  int blockId = 0;
  vtkCTHData* output = this->GetOutput();

  // This is 10x10x10 in cells.
  int dim = this->Dimensions;
  float xOrigin = -1.75;
  float yOrigin = -1.25;
  float zOrigin = 0.0;
  float xSize = 2.5;
  float ySize = 2.5;
  float zSize = 2.0;

  float ox1, oy1, oz1;
  float sx1, sy1, sz1;
  float mx1, my1, mz1;

  float ox2, oy2, oz2;
  float sx2, sy2, sz2;
  float mx2, my2, mz2;

  this->SetDimensions(dim+1, dim+1, dim+1);
  // Each refinement adds seven (adds eight takes away 1).
  output->SetNumberOfBlocks(15);

  // Lowest resolution 8
  sx1 = xSize / (2.0 * dim);
  sy1 = ySize / (2.0 * dim);
  sz1 = zSize / (2.0 * dim);
  ox1 = xOrigin;
  oy1 = yOrigin;
  oz1 = zOrigin;

  sx2 = 0.5 * sx1;
  sy2 = 0.5 * sy1;
  sz2 = 0.5 * sz1;

  mx1 = ox1 + xSize / 2.0;
  my1 = oy1 + ySize / 2.0;
  mz1 = oz1 + zSize / 2.0;
  
  this->SetBlockInfo(blockId++, ox1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, oy1, mz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, mz1, sx1, sy1, sz1);
  //this->SetBlockInfo(blockId++, ox1, my1, mz1, sx1, sy1, sz1);
  // Refine -x, +y, +z
  ox2 = ox1;
  oy2 = my1;
  oz2 = mz1;
  mx2 = ox2 + xSize / 4.0;
  my2 = oy2 + ySize / 4.0;
  mz2 = oz2 + zSize / 4.0;
    this->SetBlockInfo(blockId++, ox2, oy2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, oy2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, ox2, my2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, my2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, ox2, oy2, mz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, oy2, mz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, ox2, my2, mz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, my2, mz2, sx2, sy2, sz2);
  this->SetBlockInfo(blockId++, mx1, my1, mz1, sx1, sy1, sz1);

  this->AddFractalArray();
  this->AddBlockIdArray();
  this->AddDepthArray(sx1);

  if (this->GhostLevels > 0)
    {
    this->AddGhostLevelArray();
    }
}
*/
//----------------------------------------------------------------------------
void vtkCTHFractal::Execute()
{
  int blockId = 0;
  vtkCTHData* output = this->GetOutput();

  // This is 10x10x10 in cells.
  int dim = this->Dimensions;
  float xOrigin = -1.75;
  float yOrigin = -1.25;
  float zOrigin = 0.0;
  float xSize = 2.5;
  float ySize = 2.5;
  float zSize = 2.0;

  float ox1, oy1, oz1;
  float sx1, sy1, sz1;
  float mx1, my1, mz1;

  float ox2, oy2, oz2;
  float sx2, sy2, sz2;
  float mx2, my2, mz2;

  float ox3, oy3, oz3;
  float sx3, sy3, sz3;
  float mx3, my3, mz3;

  float ox4, oy4, oz4;
  float sx4, sy4, sz4;
  float mx4, my4, mz4;

  float ox5, oy5, oz5;
  float sx5, sy5, sz5;
  float mx5, my5, mz5;

  this->SetDimensions(dim+1, dim+1, dim+1);
  // Each refinement adds seven (adds eight takes away 1).
  output->SetNumberOfBlocks(57);

  // Lowest resolution 8
  sx1 = xSize / (2.0 * dim);
  sy1 = ySize / (2.0 * dim);
  sz1 = zSize / (2.0 * dim);

  sx2 = sx1 / 2.0;
  sy2 = sy1 / 2.0;
  sz2 = sz1 / 2.0;

  sx3 = sx2 / 2.0;
  sy3 = sy2 / 2.0;
  sz3 = sz2 / 2.0;

  sx4 = sx3 / 2.0;
  sy4 = sy3 / 2.0;
  sz4 = sz3 / 2.0;

  sx5 = sx4 / 2.0;
  sy5 = sy4 / 2.0;
  sz5 = sz4 / 2.0;

  ox1 = xOrigin;
  oy1 = yOrigin;
  oz1 = zOrigin;

  mx1 = ox1 + xSize / 2.0;
  my1 = oy1 + ySize / 2.0;
  mz1 = oz1 + zSize / 2.0;
  
  this->SetBlockInfo(blockId++, ox1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, my1, oz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, ox1, oy1, mz1, sx1, sy1, sz1);
  this->SetBlockInfo(blockId++, mx1, oy1, mz1, sx1, sy1, sz1);
  //this->SetBlockInfo(blockId++, ox1, my1, mz1, sx1, sy1, sz1);
  // Refine -x, +y, +z
  ox2 = ox1;
  oy2 = my1;
  oz2 = mz1;
  mx2 = ox2 + xSize / 4.0;
  my2 = oy2 + ySize / 4.0;
  mz2 = oz2 + zSize / 4.0;
    //this->SetBlockInfo(blockId++, ox2, oy2, oz2, sx2, sy2, sz2);
    // Refine -x, -y, -z
    ox3 = ox2;
    oy3 = oy2;
    oz3 = oz2;
    mx3 = ox3 + xSize / 8.0;
    my3 = oy3 + ySize / 8.0;
    mz3 = oz3 + zSize / 8.0;
      //this->SetBlockInfo(blockId++, ox3, oy3, oz3, sx3, sy3, sz3);
      // Refine -x, -y, -z
      ox4 = ox3;
      oy4 = oy3;
      oz4 = oz3;
      mx4 = ox4 + xSize / 16.0;
      my4 = oy4 + ySize / 16.0;
      mz4 = oz4 + zSize / 16.0;
        this->SetBlockInfo(blockId++, ox4, oy4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, oy4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, ox4, my4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, my4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, ox4, oy4, mz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, oy4, mz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, ox4, my4, mz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, my4, mz4, sx4, sy4, sz4);
      this->SetBlockInfo(blockId++, mx3, oy3, oz3, sx3, sy3, sz3);
      //this->SetBlockInfo(blockId++, ox3, my3, oz3, sx3, sy3, sz3);
      // Refine -x, +y, -z
      ox4 = ox3;
      oy4 = my3;
      oz4 = oz3;
      mx4 = ox4 + xSize / 16.0;
      my4 = oy4 + ySize / 16.0;
      mz4 = oz4 + zSize / 16.0;
        this->SetBlockInfo(blockId++, ox4, oy4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, oy4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, ox4, my4, oz4, sx4, sy4, sz4);
        this->SetBlockInfo(blockId++, mx4, my4, oz4, sx4, sy4, sz4);
        //this->SetBlockInfo(blockId++, ox4, oy4, mz4, sx4, sy4, sz4);
        // Refine -x, -y, +z
        ox5 = ox4;
        oy5 = oy4;
        oz5 = mz4;
        mx5 = ox5 + xSize / 32.0;
        my5 = oy5 + ySize / 32.0;
        mz5 = oz5 + zSize / 32.0;
          this->SetBlockInfo(blockId++, ox5, oy5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, oy5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, my5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, my5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, oy5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, oy5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, my5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, my5, mz5, sx5, sy5, sz5);
        this->SetBlockInfo(blockId++, mx4, oy4, mz4, sx4, sy4, sz4);
        //this->SetBlockInfo(blockId++, ox4, my4, mz4, sx4, sy4, sz4);
        // Refine -x, +y, +z
        ox5 = ox4;
        oy5 = my4;
        oz5 = mz4;
        mx5 = ox5 + xSize / 32.0;
        my5 = oy5 + ySize / 32.0;
        mz5 = oz5 + zSize / 32.0;
          this->SetBlockInfo(blockId++, ox5, oy5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, oy5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, my5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, my5, oz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, oy5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, oy5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, ox5, my5, mz5, sx5, sy5, sz5);
          this->SetBlockInfo(blockId++, mx5, my5, mz5, sx5, sy5, sz5);
        this->SetBlockInfo(blockId++, mx4, my4, mz4, sx4, sy4, sz4);
      this->SetBlockInfo(blockId++, mx3, my3, oz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, ox3, oy3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, oy3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, ox3, my3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, my3, mz3, sx3, sy3, sz3);
    this->SetBlockInfo(blockId++, mx2, oy2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, ox2, my2, oz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, my2, oz2, sx2, sy2, sz2);
    //this->SetBlockInfo(blockId++, ox2, oy2, mz2, sx2, sy2, sz2);
    // Refine -x, -y, +z
    ox3 = ox2;
    oy3 = oy2;
    oz3 = mz2;
    mx3 = ox3 + xSize / 8.0;
    my3 = oy3 + ySize / 8.0;
    mz3 = oz3 + zSize / 8.0;
      this->SetBlockInfo(blockId++, ox3, oy3, oz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, oy3, oz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, ox3, my3, oz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, my3, oz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, ox3, oy3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, oy3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, ox3, my3, mz3, sx3, sy3, sz3);
      this->SetBlockInfo(blockId++, mx3, my3, mz3, sx3, sy3, sz3);
    this->SetBlockInfo(blockId++, mx2, oy2, mz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, ox2, my2, mz2, sx2, sy2, sz2);
    this->SetBlockInfo(blockId++, mx2, my2, mz2, sx2, sy2, sz2);
  this->SetBlockInfo(blockId++, mx1, my1, mz1, sx1, sy1, sz1);

  this->AddFractalArray();
  this->AddBlockIdArray();
  this->AddDepthArray(sx1);

  if (this->GhostLevels > 0)
    {
    this->AddGhostLevelArray();
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
  
  array->SetName("Fractal");
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
void vtkCTHFractal::AddDepthArray(float sx1)
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
    depth = (int)((sx1 / spacing[0]) + 0.5);
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
}

