/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHExtractAMRPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkImageData.h"
#include "vtkCharArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkOutlineFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyData.h"
#include "vtkClipPolyData.h"
#ifdef VTK_USE_PATENTED
#  include "vtkPVKitwareContourFilter.h"
#  include "vtkKitwareCutter.h"
#else
#  include "vtkContourFilter.h"
#  include "vtkCutter.h"
#endif
#include "vtkStringList.h"
#include "vtkPlane.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"



vtkCxxRevisionMacro(vtkCTHExtractAMRPart, "1.10");
vtkStandardNewMacro(vtkCTHExtractAMRPart);
vtkCxxSetObjectMacro(vtkCTHExtractAMRPart,ClipPlane,vtkPlane);

//----------------------------------------------------------------------------
vtkCTHExtractAMRPart::vtkCTHExtractAMRPart()
{
  this->VolumeArrayNames = vtkStringList::New();
  //this->ClipPlane = vtkPlane::New();
  // For consistent references.
  //this->ClipPlane->Register(this);
  //this->ClipPlane->Delete();
  this->ClipPlane = 0;

  // So we do not have to keep creating idList in a loop of Execute.
  this->IdList = vtkIdList::New();

  this->Image = 0;
  this->PolyData = 0;
  this->Contour = 0;
  this->Append1 = 0;
  this->Append2 = 0;
  this->Surface = 0;
  this->Clip0 = 0;
  this->Clip1 = 0;
  this->Clip2 =0;
  this->Cut = 0;
  this->FinalAppend = 0;
}

//----------------------------------------------------------------------------
vtkCTHExtractAMRPart::~vtkCTHExtractAMRPart()
{
  this->VolumeArrayNames->Delete();
  this->VolumeArrayNames = NULL;
  this->SetClipPlane(NULL);

  this->IdList->Delete();
  this->IdList = NULL;

  // Not really necessary because exeucte cleans up.
  this->DeleteInternalPipeline();
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If clip plane is modified,
// then this object is modified as well.
unsigned long vtkCTHExtractAMRPart::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ClipPlane)
    {
    time = this->ClipPlane->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//--------------------------------------------------------------------------
void vtkCTHExtractAMRPart::RemoveAllVolumeArrayNames()
{
  int num, idx;

  num = this->GetNumberOfOutputs();
  for (idx = 0; idx < num; ++idx)
    {
    this->SetOutput(idx, NULL);
    }

  this->VolumeArrayNames->RemoveAllItems();  
}

//--------------------------------------------------------------------------
void vtkCTHExtractAMRPart::AddVolumeArrayName(char* arrayName)
{
  vtkPolyData* d = vtkPolyData::New();
  int num = this->GetNumberOfOutputs();
  this->VolumeArrayNames->AddString(arrayName);
  this->SetOutput(num, d);
  d->Delete();
  d = NULL;
}

//--------------------------------------------------------------------------
int vtkCTHExtractAMRPart::GetNumberOfVolumeArrayNames()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}

//--------------------------------------------------------------------------
const char* vtkCTHExtractAMRPart::GetVolumeArrayName(int idx)
{
  return this->VolumeArrayNames->GetString(idx);
}

//--------------------------------------------------------------------------
void vtkCTHExtractAMRPart::SetOutput(int idx, vtkPolyData* d)
{
  this->vtkSource::SetNthOutput(idx, d);  
}

//----------------------------------------------------------------------------
vtkPolyData* vtkCTHExtractAMRPart::GetOutput(int idx)
{
  return (vtkPolyData *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
int vtkCTHExtractAMRPart::GetNumberOfOutputs()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}


//----------------------------------------------------------------------------
void vtkCTHExtractAMRPart::Execute()
{
  int blockId, numBlocks;
  vtkCTHData* input = this->GetInput();
  vtkCTHData* inputCopy = vtkCTHData::New();
  vtkPolyData* output;
  vtkImageData* block; 
  vtkPolyData** tmps;
  int idx, num;

  this->CreateInternalPipeline();
  inputCopy->ShallowCopy(input);

  vtkTimerLog::MarkStartEvent("CellToPoint");

  // If there are no ghost cells, then try our fancy way of
  // computing cell volume fractions.  It finds all point cells
  // including cells from neighboring blocks that touch the point.
  if (inputCopy->GetNumberOfGhostLevels() == 0)
    {
    // Loop over parts to convert volume fractions to point arrays.
    num = this->VolumeArrayNames->GetNumberOfStrings();
    for (idx = 0; idx < num; ++idx)
      {
      vtkFloatArray* pointVolumeFraction;
      vtkDataArray* cellVolumeFraction;
      const char* arrayName = this->VolumeArrayNames->GetString(idx);

      cellVolumeFraction = inputCopy->GetCellData()->GetArray(arrayName);
      if (cellVolumeFraction == NULL)
        {
        vtkErrorMacro("Could not find cell array " << arrayName);
        inputCopy->Delete();
        return;
        }
      if (cellVolumeFraction->GetDataType() != VTK_FLOAT)
        {
        vtkErrorMacro("Expecting volume fraction to be of type float.");
        inputCopy->Delete();
        return;
        }

      pointVolumeFraction = vtkFloatArray::New();
      pointVolumeFraction->SetNumberOfTuples(inputCopy->GetNumberOfPoints());

      this->ExecuteCellDataToPointData2(cellVolumeFraction, 
                                        pointVolumeFraction, 
                                        inputCopy);
      inputCopy->GetPointData()->AddArray(pointVolumeFraction);
      pointVolumeFraction->Delete();
      inputCopy->GetCellData()->RemoveArray(arrayName);
      }
    } 

  vtkTimerLog::MarkEndEvent("CellToPoint");

  // Create an append for each part (one part per output).
  num = this->VolumeArrayNames->GetNumberOfStrings();
  tmps = new vtkPolyData* [num];
  for (idx = 0; idx < num; ++idx)
    {
    tmps[idx] = vtkPolyData::New();
    }

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = inputCopy->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    block = vtkImageData::New();
    inputCopy->GetBlock(blockId, block);
    this->ExecuteBlock(block, tmps);
    block->Delete();
    block = NULL;
    }
  
  // Copy appends to output (one part per output).
  for (idx = 0; idx < num; ++idx)
    {
    output = this->GetOutput(idx);
    output->ShallowCopy(tmps[idx]);
    tmps[idx]->Delete();
    tmps[idx] = NULL;

    // In the future we might be able to select the rgb color here.
    if (num > 1)
      {
      // Add scalars to color this part.
      int numPts = output->GetNumberOfPoints();
      vtkFloatArray *partArray = vtkFloatArray::New();
      partArray->SetName("Part Index");
      float *p = partArray->WritePointer(0, numPts);
      for (int idx2 = 0; idx2 < numPts; ++idx2)
        {
        p[idx2] = (float)(idx);
        }
      output->GetPointData()->SetScalars(partArray);
      partArray->Delete();
      }

    // Add a name for this part.
    vtkCharArray *nameArray = vtkCharArray::New();
    nameArray->SetName("Name");
    const char* arrayName = this->VolumeArrayNames->GetString(idx);
    char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
    sprintf(str, "%s", arrayName);
    output->GetFieldData()->AddArray(nameArray);
    nameArray->Delete();

    // Get rid of extra ghost levels.
    output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);
    }
  delete [] tmps;
  tmps = NULL;
  inputCopy->Delete();
  inputCopy = NULL;
  this->DeleteInternalPipeline();
}

//-----------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecuteBlock(vtkImageData* block, 
                                        vtkPolyData** tmps)
{
  vtkFloatArray* pointVolumeFraction;
  int idx, num;
  const char* arrayName;
  int *dims;

  // Loop over parts to convert volume fractions to point arrays.
  num = this->VolumeArrayNames->GetNumberOfStrings();
  for (idx = 0; idx < num; ++idx)
    {
    arrayName = this->VolumeArrayNames->GetString(idx);
    pointVolumeFraction = (vtkFloatArray*)(block->GetPointData()->GetArray(arrayName));
    if (pointVolumeFraction == NULL)
      {
      vtkDataArray* cellVolumeFraction;
      dims = block->GetDimensions();

      cellVolumeFraction = block->GetCellData()->GetArray(arrayName);
      if (cellVolumeFraction == NULL)
        {
        vtkErrorMacro("Could not find cell array " << arrayName);
        return;
        }
      if (cellVolumeFraction->GetDataType() != VTK_FLOAT)
        {
        vtkErrorMacro("Expecting volume fraction to be of type float.");
        return;
        }

      pointVolumeFraction = vtkFloatArray::New();
      pointVolumeFraction->SetNumberOfTuples(block->GetNumberOfPoints());

      this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                       pointVolumeFraction, 
                                       dims);
      block->GetPointData()->AddArray(pointVolumeFraction);
      pointVolumeFraction->Delete();
      block->GetCellData()->RemoveArray(arrayName);
      }
    } 

  // Get rid of ghost cells.
  int ext[6];
  int extraGhostLevels = this->GetInput()->GetNumberOfGhostLevels() - this->GetOutput()->GetUpdateGhostLevel();
  block->GetExtent(ext);
  ext[0] += extraGhostLevels;
  ext[2] += extraGhostLevels;
  ext[4] += extraGhostLevels;
  ext[1] -= extraGhostLevels;
  ext[3] -= extraGhostLevels;
  ext[5] -= extraGhostLevels;
  block->SetUpdateExtent(ext);
  block->Crop();
  
  // Loop over parts extracting surfaces.
  for (idx = 0; idx < num; ++idx)
    {
    arrayName = this->VolumeArrayNames->GetString(idx);
    this->ExecutePart(arrayName, block, tmps[idx]);
    } 
}


//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::CreateInternalPipeline()
{
  // Having inputs keeps us from having to set and remove inputs.
  // The garbage collecting associated with this is expensive.
  this->Image = vtkImageData::New();
  this->PolyData = vtkPolyData::New();
  
  // The filter that iteratively appends the output.
  this->FinalAppend = vtkAppendPolyData::New();
  
  // Create the contour surface.
#ifdef VTK_USE_PATENTED
  this->Contour = vtkPVKitwareContourFilter::New();
  // vtkDataSetSurfaceFilter does not generate normals, so they will be lost.
  contour->ComputeNormalsOff();
#else
  this->Contour = vtkContourFilter::New();
#endif
  this->Contour->SetInput(this->Image);
  this->Contour->SetValue(0, 0.5);

  // Create the capping surface for the contour and append.
  this->Append1 = vtkAppendPolyData::New();
  this->Append1->AddInput(this->Contour->GetOutput());
  this->Surface = vtkDataSetSurfaceFilter::New();
  this->Surface->SetInput(this->Image);

  // Clip surface less than volume fraction 0.5.
  this->Clip0 = vtkClipPolyData::New();
  this->Clip0->SetInput(this->Surface->GetOutput());
  this->Clip0->SetValue(0.5);
  this->Append1->AddInput(this->Clip0->GetOutput());
  
  if (this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    this->Append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    this->Clip1 = vtkClipPolyData::New();
    this->Clip1->SetInput(this->Append1->GetOutput());
    this->Clip1->SetClipFunction(this->ClipPlane);
    this->Append2->AddInput(this->Clip1->GetOutput());
    // We need to create a capping surface.
#ifdef VTK_USE_PATENTED
    this->Cut = vtkKitwareCutter::New();
#else
    this->Cut = vtkCutter::New();
#endif
    this->Cut->SetCutFunction(this->ClipPlane);
    this->Cut->SetValue(0, 0.0);
    this->Cut->SetInput(this->Image);
    this->Clip2 = vtkClipPolyData::New();
    this->Clip2->SetInput(this->Cut->GetOutput());
    this->Clip2->SetValue(0.5);
    this->Append2->AddInput(this->Clip2->GetOutput());
    this->Append2->Update();
    this->FinalAppend->AddInput(this->Append2->GetOutput());
    }
  else
    {
    this->FinalAppend->AddInput(this->Append1->GetOutput());
    }
  this->FinalAppend->AddInput(this->PolyData);
}

//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::DeleteInternalPipeline()
{
  if (this->Image)
    {
    this->Image->Delete();
    this->Image = 0;
    }
  if (this->PolyData)
    {
    this->PolyData->Delete();
    this->PolyData = 0;
    }

  if (this->Clip1)
    {
    this->Clip1->Delete();
    this->Clip1 = 0;
    }
  if (this->Cut)
    {
    this->Cut->Delete();
    this->Cut = 0;
    }
  if (this->Clip2)
    {
    this->Clip2->Delete();
    this->Clip2 = 0;
    }
  if (this->Contour)
    {
    this->Contour->Delete();
    this->Contour = 0;
    }
  if (this->Surface)
    {
    this->Surface->Delete();
    this->Surface = 0;
    }
  if (this->Clip0)
    {
    this->Clip0->Delete();
    this->Clip0 = 0;
    }
  if (this->Append1)
    {
    // Make sure all temporary fitlers actually delete.
    this->Append1->SetOutput(NULL);
    this->Append1->Delete();
    this->Append1 = 0;
    }
  if (this->Append2)
    {
    // Make sure all temperary fitlers actually delete.
    this->Append2->SetOutput(NULL);
    this->Append2->Delete();
    this->Append2 = 0;
    }
  if (this->FinalAppend)
    {
    this->FinalAppend->Delete();
    this->FinalAppend = 0;
    }
}

//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecutePart(const char* arrayName,
                                       vtkImageData* block, 
                                       vtkPolyData* appendCache)
{
  block->GetPointData()->SetActiveScalars(arrayName);

  this->Image->ShallowCopy(block);
  this->PolyData->ShallowCopy(appendCache);
  this->FinalAppend->Update();
  appendCache->ShallowCopy(this->FinalAppend->GetOutput());
}

//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, int *dims)
{
  int count;
  int i, j, k;
  int iEnd, jEnd, kEnd;
  int jInc, kInc;
  float *pPoint;
  float *pCell;

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());

  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;
  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;
  
  pPoint = pointVolumeFraction->GetPointer(0);
  pCell = (float*)(cellVolumeFraction->GetVoidPointer(0));

  // Initialize the point data to 0.
  memset(pPoint, 0,  dims[0]*dims[1]*dims[2]*sizeof(float));

  // Loop thorugh the cells.
  for (k = 0; k < kEnd; ++k)
    {
    for (j = 0; j < jEnd; ++j)
      {
      for (i = 0; i < iEnd; ++i)
        {
        // Add cell value to all points of cell.
        *pPoint += *pCell;
        pPoint[1] += *pCell;
        pPoint[jInc] += *pCell;
        pPoint[1+jInc] += *pCell;
        pPoint[kInc] += *pCell;
        pPoint[kInc+1] += *pCell;
        pPoint[kInc+jInc] += *pCell;
        pPoint[kInc+jInc+1] += *pCell;

        // Increment pointers
        ++pPoint;
        ++pCell;
        }
      // Skip over last point to the start of the next row.
      ++pPoint;
      }
    // Skip over the last row to the start of the next plane.
    pPoint += jInc;
    }

  // Now a second pass to normalize the point values.
  // Loop through the points.
  count = 1;
  pPoint = pointVolumeFraction->GetPointer(0);
  for (k = 0; k <= kEnd; ++k)
    {
    // Just a fancy fast way to compute the number of cell neighbors of a point.
    if (k == 1)
      {
      count = count << 1;
      }
    if (k == kEnd)
      {
      count = count >> 1;
      }
    for (j = 0; j <= jEnd; ++j)
      {
      // Just a fancy fast way to compute the number of cell neighbors of a point.
      if (j == 1)
        {
        count = count << 1;
        }
      if (j == jEnd)
        {
        count = count >> 1;
        }
      for (i = 0; i <= iEnd; ++i)
        {
        // Just a fancy fast way to compute the number of cell neighbors of a point.
        if (i == 1)
          {
          count = count << 1;
          }
        if (i == iEnd)
          {
          count = count >> 1;
          }
        *pPoint = *pPoint / (float)(count);
        ++pPoint;
        }
      }
    }
}

//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point ,verage all cells touching the point.
void vtkCTHExtractAMRPart::ExecuteCellDataToPointData2(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, vtkCTHData* data)
{
  int x, y, z, maxX, maxY, maxZ;
  float *pPoint0;
  float *pPoint;
  float *pCell0;
  float *pCell;
  int cInc[6];
  int pIncX, pIncY, pIncZ;
  int blockId, numBlocks;
  vtkIdList* blockList = vtkIdList::New();
  // hack
  int *dims = data->GetBlockPointDimensions(0);
  pIncX = 1;
  pIncY = dims[0];
  pIncZ = pIncY * dims[1];
  int numCellsPerBlock = data->GetNumberOfCellsPerBlock();
  int numPtsPerBlock = data->GetNumberOfPointsPerBlock();

  // All neighbor cell incs except 0 and 1.
  // Funny order is for cache locality.
  cInc[0] = dims[0] - 1;              // +y 
  cInc[2] = (dims[1]-1) * (dims[0]-1);// +z
  cInc[1] = 1 + cInc[0];              // +x +y
  cInc[3] = 1 + cInc[2];              // +x +z
  cInc[4] = cInc[0] + cInc[2];        // +y +z
  cInc[5] = 1 + cInc[0] + cInc[2];    // +x +y +z

  maxX = dims[0]-1;
  maxY = dims[1]-1;
  maxZ = dims[2]-1;

  // It might be faster to have a separate loop for interior points.
  // Loop over all points.
  pPoint0 = pointVolumeFraction->GetPointer(0);
  pCell0 = (float*)(cellVolumeFraction->GetVoidPointer(0));
  numBlocks = data->GetNumberOfBlocks();

  // Now lets loop over blocks. Low levels first.
  // Processing the low levels first will allow me to force some
  // boundary points (that are not on neighbor grid points) to obey
  // linear interpolation of neighbor's face.
  int blocksFinished = 0;
  int level = 0;
  while (blocksFinished < numBlocks)
    { // Look for all blocks with level.
    for (blockId = 0; blockId < numBlocks; ++blockId)
      {
      // We should really have level a better part of the data set structure.
      // Look to level array for now.
      if (data->GetBlockLevel(blockId) < 0)
        {
        vtkErrorMacro("Bad block level");
        // avoids an infinite loop.
        ++blocksFinished;
        }
      if (data->GetBlockLevel(blockId) == level)
        {
        // Mark a block as finished (before we start processing the block).
        ++blocksFinished;
        // Move pointers to start of block.
        pCell = pCell0 + (blockId*numCellsPerBlock);
        pPoint = pPoint0 + (blockId*numPtsPerBlock);
        // Make a list of neighbors so each point does not need to look through
        // all the blocks.  With a better data structure, we might have
        // a method to return neighbors of indexed by block face.
        this->FindBlockNeighbors(data, blockId, blockList);
        // Loop over all points in this block.
        for (z = 0; z <= maxZ; ++z)
          {
          for (y = 0; y <= maxY; ++y)
            {
            for (x = 0; x <= maxX; ++x)
              {
              if (z == 0 || y == 0 || x == 0)
                {
                *pPoint = this->ComputeSharedPoint(blockId, blockList, 
                                                   x, y, z, pCell0, pPoint0, data);
                ++pPoint;
                // Do not increment the cell pointer for negative boundary faces.
                }
              else if (z == maxZ || y == maxY || x == maxX)
                {
                *pPoint = this->ComputeSharedPoint(blockId, blockList, 
                                                   x, y, z, pCell0, pPoint0, data);
                ++pPoint;
                ++pCell;
                }
              else
                {
                // This fast path for interior point should speed things up.
                // Average the eight neighboring cells.
                *pPoint = (pCell[0] + pCell[1] + pCell[cInc[0]] + pCell[cInc[1]]
                         + pCell[cInc[2]] + pCell[cInc[3]] + pCell[cInc[4]]
                         + pCell[cInc[5]]) * 0.125;
                ++pPoint;
                ++pCell;
                }      
              } // x loop
            } // y loop
          } // z loop
        } // if (data->GetBlockLevel(blockId) == level)
      } // for (blockId = 0; blockId < numBlocks; ++blockId)
      // Move to next level.
      ++level;
    } // while (blocksFinished < numBlocks)
  blockList->Delete();
  pointVolumeFraction->SetName(cellVolumeFraction->GetName());
}



//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point, average all cells touching the point.
// Weight the cell by inverse of spacing.
// We need the point array only for linear interpolation constraint.
float vtkCTHExtractAMRPart::ComputeSharedPoint(int blockId, vtkIdList* blockList,
                                               int x, int y, int z, 
                                               float* pCell, float* pPoint, 
                                               vtkCTHData* input)
{
  // hack
  int* dims = input->GetBlockPointDimensions(0);
  double* spacing;
  double sum = 0.0;
  double weight;
  double sumWeight = 0.0;
  double epsilon;
  double* origin;
  double outside[3];
  int numPtsPerBlock = input->GetNumberOfPointsPerBlock();
  int numCellsPerBlock = input->GetNumberOfCellsPerBlock();
  int i, id, num;
  int x0, x1, y0, y1, z0, z1;
  double dx, dy, dz;
  double pt[3];
  int pMaxX = dims[0]-1;
  int pMaxY = dims[1]-1;
  int pMaxZ = dims[2]-1;
  //int cMaxX = pMaxX-1;
  //int cMaxY = pMaxY-1;
  //int cMaxZ = pMaxZ-1;
  int cIncY = dims[0]-1;
  int cIncZ = (dims[1]-1)*cIncY;

  // First add cells local to block.
  spacing = input->GetBlockSpacing(blockId);
  // Assume x, y and z spacing are the same.
  weight = 1.0/spacing[0];
  id = (blockId * numCellsPerBlock) + x + y*cIncY + z*cIncZ;
  if (x > 0 && y > 0 && z > 0)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncY - cIncZ];
    }
  if (x < pMaxX && y > 0 && z > 0)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncY - cIncZ];
    }
  if (x > 0 && y < pMaxY && z > 0)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncZ];
    }
  if (x < pMaxX && y < pMaxY && z > 0)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncZ];
    }
  if (x > 0 && y > 0 && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncY];
    }
  if (x < pMaxX && y > 0 && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncY];
    }
  if (x > 0 && y < pMaxY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1];
    }
  if (x < pMaxX && y < pMaxY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id];
    }

  // Next find all the block that share the point.
  // Compute point in world space.
  origin = input->GetBlockOrigin(blockId);
  pt[0] = origin[0] + (double)x * spacing[0];
  pt[1] = origin[1] + (double)y * spacing[1];
  pt[2] = origin[2] + (double)z * spacing[2];
  epsilon = spacing[0] / 1000.0;

  num = blockList->GetNumberOfIds();
  for (i = 0; i < num; ++i)
    {
    id = blockList->GetId(i);
    origin = input->GetBlockOrigin(id);
    if (pt[0] > origin[0]-epsilon && pt[1] > origin[1]-epsilon &&
        pt[2] > origin[2]-epsilon)
      {
      spacing = input->GetBlockSpacing(id);
      weight = 1.0/spacing[0];
      outside[0] = origin[0] + spacing[0]*(double)(dims[0]-1);
      outside[1] = origin[1] + spacing[1]*(double)(dims[1]-1);
      outside[2] = origin[2] + spacing[2]*(double)(dims[2]-1);
      if (pt[0] < outside[0]+epsilon && pt[1] < outside[1]+epsilon &&
          pt[2] < outside[2]+epsilon)
        { // Point is contained in block.
        // Compute point index.
        x = (int)((pt[0]+epsilon - origin[0]) / spacing[0]);
        y = (int)((pt[1]+epsilon - origin[1]) / spacing[1]);
        z = (int)((pt[2]+epsilon - origin[2]) / spacing[2]);
        // Sanity check:  We expect that blocks only share faces.
        if (x>0 && x<pMaxX && y>0 && y<pMaxY && z>0 && z<pMaxZ)
          {
          vtkErrorMacro("Expecting a boundary point.");
          }
        // Compute remainder (whether point is on grid).
        // This should really just be a flag because
        // we do not use the remainder for interpolation.
        // We just assume that offgrid point fall in middle
        // (0.5) of faces.
        dx = pt[0] - origin[0] - (spacing[0] * (double)x);
        dy = pt[1] - origin[1] - (spacing[1] * (double)y);
        dz = pt[2] - origin[2] - (spacing[2] * (double)z);

        // This assumes neighbor blocks differ only by one level.
        // Here is where we force points to obey linear interpolation
        // of neighbor.  This will produce an iso surface with no cracks.
        // This is not necessary if point lies on neighbors grid.
        if (dx > epsilon || dy > epsilon || dz > epsilon)
          {
          int pIncZ = dims[0]*dims[1];
          float* cornerPoint = pPoint + id*numPtsPerBlock + x + y*dims[0] + z*pIncZ;
          // Since there are only six cases (3 edges and 3 faces),
          // just have a condition for each.
          if (dx > epsilon && dy <= epsilon && dz <= epsilon)
            { // x edge
            return 0.5 * (cornerPoint[0] + cornerPoint[1]);
            }
          else if (dx <= epsilon && dy > epsilon && dz <= epsilon)
            { // y edge
            return 0.5 * (cornerPoint[0] + cornerPoint[dims[0]]);
            }
          else if (dx <= epsilon && dy <= epsilon && dz > epsilon)
            { // z edge
            return 0.5 * (cornerPoint[0] + cornerPoint[pIncZ]);
            }
          else if (dx > epsilon && dy > epsilon && dz <= epsilon)
            { // xy face
            return 0.25 * (cornerPoint[0] + cornerPoint[1] + 
                           cornerPoint[dims[0]] + cornerPoint[1+dims[0]]);
            }
          else if (dx > epsilon && dy <= epsilon && dz > epsilon)
            { // xz face
            return 0.25 * (cornerPoint[0] + cornerPoint[1] + 
                           cornerPoint[pIncZ] + cornerPoint[1+pIncZ]);
            }
          else if (dx <= epsilon && dy > epsilon && dz > epsilon)
            { // yz face
            return 0.25 * (cornerPoint[0] + cornerPoint[dims[0]] + 
                           cornerPoint[pIncZ] + cornerPoint[dims[0]+pIncZ]);
            }
          else
            { // Point in middle of cell.  Drop through.
            vtkErrorMacro("Bondary point in middle of neighbor cell.");
            }
          }
        // Handle last point (max boundary face).
        // Treat them like an interior (off grid) point.
        if (x == pMaxX) {x = pMaxX-1; dx += spacing[0];}
        if (y == pMaxY) {y = pMaxY-1; dy += spacing[0];}
        if (z == pMaxZ) {z = pMaxZ-1; dz += spacing[0];}

        // Compute the extent (min/max block) of cells touching point. 
        x0 = x1 = x;
        y0 = y1 = y;
        z0 = z1 = z;
        if (dx < epsilon && x > 0) {--x0;}
        if (dy < epsilon && y > 0) {--y0;}
        if (dz < epsilon && z > 0) {--z0;}
        // Now loop over cells adding to idList.
        for (z = z0; z <= z1; ++z)
          {
          for (y = y0; y <= y1; ++y)
            {
            for (x = x0; x <= x1; ++x)
              {
              sumWeight += weight;
              sum += weight*pCell[id*numCellsPerBlock 
                                  + x + y*cIncY + z*cIncZ];
              }
            }
          }
        } // End: < outside (pt in block bounds)
      } // End: > origin (min bounds).
    } // End: block loop.

  //  average cells values.
  return sum / sumWeight; 
}

/*
//------------------------------------------------------------------------------
// Should really be in the data object.
// Returns cells even if a face contains the point.
void vtkCTHExtractAMRPart::FindPointCells(vtkCTHData* self, vtkIdType ptId, 
                                          vtkIdList* idList)
{
  float epsilon;
  float* origin;
  float outside[3];
  float* spacing;
  int numPtsPerBlock = self->GetNumberOfPointsPerBlock();
  int numCellsPerBlock = self->GetNumberOfCellsPerBlock();
  int id, num, blockId, tmp, x, y, z;
  int x0, x1, y0, y1, z0, z1;
  float dx, dy, dz;
  float pt[3];
  int* dims = self->GetPointDimensions();
  int pMaxX = dims[0]-1;
  int pMaxY = dims[1]-1;
  int pMaxZ = dims[2]-1;
  int cMaxX = pMaxX-1;
  int cMaxY = pMaxY-1;
  int cMaxZ = pMaxZ-1;
  int cIncY = dims[0]-1;
  int cIncZ = (dims[1]-1)*cIncY;

  idList->Initialize();

  // First add cells local to block.
  blockId = ptId / numPtsPerBlock;
  tmp = ptId - blockId*numPtsPerBlock;
  z = tmp / (dims[0]*dims[1]);
  tmp = tmp - z*dims[0]*dims[1];
  y = tmp / dims[0];
  x = tmp - y*dims[0];

  id = (blockId * numCellsPerBlock) + x + y*cIncY + z*cIncZ;
  if (x > 0 && y > 0 && z > 0)
    {
    idList->InsertNextId(id - 1 - cIncY - cIncZ);
    }
  if (x < pMaxX && y > 0 && z > 0)
    {
    idList->InsertNextId(id - cIncY - cIncZ);
    }
  if (x > 0 && y < pMaxY && z > 0)
    {
    idList->InsertNextId(id - 1 - cIncZ);
    }
  if (x < pMaxX && y < pMaxY && z > 0)
    {
    idList->InsertNextId(id - cIncZ);
    }
  if (x > 0 && y > 0 && z < pMaxZ)
    {
    idList->InsertNextId(id - 1 - cIncY);
    }
  if (x < pMaxX && y > 0 && z < pMaxZ)
    {
    idList->InsertNextId(id - cIncY);
    }
  if (x > 0 && y < pMaxY && z < pMaxZ)
    {
    idList->InsertNextId(id - 1);
    }
  if (x < pMaxX && y < pMaxY && z < pMaxZ)
    {
    idList->InsertNextId(id);
    }

  // Next find all the block that share the point.
  if (idList->GetNumberOfIds() == 8)
    { // Interior point.
    return;
    }
  // Compute point in world space.
  origin = self->GetBlockOrigin(blockId);
  spacing = self->GetBlockSpacing(blockId);
  pt[0] = origin[0] + (float)x * spacing[0];
  pt[1] = origin[1] + (float)y * spacing[1];
  pt[2] = origin[2] + (float)z * spacing[2];
  epsilon = spacing[0] / 1000.0;

  num = self->GetNumberOfBlocks();
  for (id = 0; id < num; ++id)
    {
    if (id != blockId)
      {  
      origin = self->GetBlockOrigin(id);
      if (pt[0] > origin[0]-epsilon && pt[1] > origin[1]-epsilon &&
          pt[2] > origin[2]-epsilon)
        {
        spacing = self->GetBlockSpacing(id);
        outside[0] = origin[0] + spacing[0]*(float)(dims[0]-1);
        outside[1] = origin[1] + spacing[1]*(float)(dims[1]-1);
        outside[2] = origin[2] + spacing[2]*(float)(dims[2]-1);
        if (pt[0] < outside[0]+epsilon && pt[1] < outside[1]+epsilon &&
            pt[2] < outside[2]+epsilon)
          { // Point is contained in block.
          // Compute point index.
          x = (int)((pt[0]+epsilon - origin[0]) / spacing[0]);
          y = (int)((pt[1]+epsilon - origin[1]) / spacing[1]);
          z = (int)((pt[2]+epsilon - origin[2]) / spacing[2]);
          // Sanity check:  We expect that blocks only share faces.
          if (x>0 && x<pMaxX && y>0 && y<pMaxY && z>0 && z<pMaxZ)
            {
            vtkErrorMacro("Expecting a boundary point.");
            }
          // Handle last point (max boundary face).
          // Treat them like an interior (off grid) point.
          if (x == pMaxX) {x = pMaxX-1;}
          if (y == pMaxY) {y = pMaxY-1;}
          if (z == pMaxZ) {z = pMaxZ-1;}
          // Compute remainder (whether point is on grid).
          dx = pt[0] - origin[0] - (spacing[0] * (float)x);
          dy = pt[1] - origin[1] - (spacing[1] * (float)y);
          dz = pt[2] - origin[2] - (spacing[2] * (float)z);
          
          // Compute the extent (min/max block) of cells touching point. 
          x0 = x1 = x;
          y0 = y1 = y;
          z0 = z1 = z;
          if (dx < epsilon && x > 0) {--x0;}
          if (dy < epsilon && y > 0) {--y0;}
          if (dz < epsilon && z > 0) {--z0;}
          // Now loop over cells adding to idList.
          for (z = z0; z <= z1; ++z)
            {
            for (y = y0; y <= y1; ++y)
              {
              for (x = x0; x <= x1; ++x)
                {
                idList->InsertNextId(id*numCellsPerBlock 
                                      + x + y*cIncY + z*cIncZ);
                }
              }
            }
          } // End: < outside (pt in block bounds)
        } // End: > origin (min bounds).
      } // End: Not same block.
    } // End: block loop.
}
*/

//------------------------------------------------------------------------------
// Note, this only works with no ghost levels.
void vtkCTHExtractAMRPart::FindBlockNeighbors(vtkCTHData* self, int blockId, vtkIdList* blockList)
{
  double *origin;
  double *spacing;
  int *dims;
  double bds0[6];
  double bds1[6];
  vtkIdType id, num;
  double e;

  blockList->Initialize();
  // hack
  dims = self->GetBlockPointDimensions(0);
  origin = self->GetBlockOrigin(blockId);
  spacing = self->GetBlockSpacing(blockId);
  bds0[0] = origin[0];
  bds0[1] = origin[0]+spacing[0]*(double)(dims[0]-1);
  bds0[2] = origin[1];
  bds0[3] = origin[1]+spacing[1]*(double)(dims[1]-1);
  bds0[4] = origin[2];
  bds0[5] = origin[2]+spacing[2]*(double)(dims[2]-1);
  // Tolerance
  e = (spacing[0]+spacing[1]+spacing[2]) / 1000.0;

  num = self->GetNumberOfBlocks();
  for (id = 0; id < num; ++id)
    {
    if (id != blockId)
      {
      origin = self->GetBlockOrigin(id);
      spacing = self->GetBlockSpacing(id);
      bds1[0] = origin[0];
      bds1[1] = origin[0]+spacing[0]*(double)(dims[0]-1);
      bds1[2] = origin[1];
      bds1[3] = origin[1]+spacing[1]*(double)(dims[1]-1);
      bds1[4] = origin[2];
      bds1[5] = origin[2]+spacing[2]*(double)(dims[2]-1);
      // Intersection of bounds
      if (bds1[0]<bds0[0]) {bds1[0] = bds0[0];}
      if (bds1[1]>bds0[1]) {bds1[1] = bds0[1];}
      if (bds1[2]<bds0[2]) {bds1[2] = bds0[2];}
      if (bds1[3]>bds0[3]) {bds1[3] = bds0[3];}
      if (bds1[4]<bds0[4]) {bds1[4] = bds0[4];}
      if (bds1[5]>bds0[5]) {bds1[5] = bds0[5];}
      // Check for overlap.
      if (bds1[0] < bds1[1]+e && bds1[2] < bds1[3]+e && bds1[4] < bds1[5]+e)
        { // All three projections are touching.
        blockList->InsertNextId(id);
        }
      }
    }
}
      

//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeArrayNames: \n";
  vtkIndent i2 = indent.GetNextIndent();
  this->VolumeArrayNames->PrintSelf(os, i2);
  if (this->ClipPlane)
    {
    os << indent << "ClipPlane:\n";
    this->ClipPlane->PrintSelf(os, indent.GetNextIndent());
    }
  else  
    {
    os << indent << "ClipPlane: NULL\n";
    }
}




