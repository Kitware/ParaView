/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRContour.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRContour.h"
#include "vtkContourValues.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

#include "vtkCTHAMRCellToPointData.h"
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



vtkCxxRevisionMacro(vtkCTHAMRContour, "1.2");
vtkStandardNewMacro(vtkCTHAMRContour);

//----------------------------------------------------------------------------
vtkCTHAMRContour::vtkCTHAMRContour()
{
  this->ContourValues = vtkContourValues::New();
  this->InputScalarsSelection = 0;
  
  // So we do not have to keep creating idList in a loop of Execute.
  this->IdList = vtkIdList::New();

  this->Image = 0;
  this->PolyData = 0;
  this->Contour = 0;
  this->FinalAppend = 0;

  this->IgnoreGhostLevels = 1;
}

//----------------------------------------------------------------------------
vtkCTHAMRContour::~vtkCTHAMRContour()
{
  this->SetInputScalarsSelection(0);
  this->ContourValues->Delete();
  this->ContourValues = 0;

  this->IdList->Delete();
  this->IdList = NULL;

  // Not really necessary because exeucte cleans up.
  this->DeleteInternalPipeline();
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If clip plane is modified,
// then this object is modified as well.
unsigned long vtkCTHAMRContour::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ContourValues)
    {
    time = this->ContourValues->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkCTHAMRContour::Execute()
{
  int blockId, numBlocks;
  vtkCTHData* input = this->GetInput();
  vtkCTHData* inputCopy = vtkCTHData::New();
  vtkPolyData* output;
  vtkImageData* block; 
  vtkPolyData* tmp;

  this->CreateInternalPipeline();
  inputCopy->ShallowCopy(input);

  vtkTimerLog::MarkStartEvent("CellToPoint");

  // If there are no ghost cells, then try our fancy way of
  // computing cell volume fractions.  It finds all point cells
  // including cells from neighboring blocks that touch the point.
  if (inputCopy->GetNumberOfGhostLevels() == 0 || this->IgnoreGhostLevels)
    {
    vtkCTHAMRCellToPointData* cellToPoint = vtkCTHAMRCellToPointData::New();
    cellToPoint->SetInput(inputCopy);
    cellToPoint->SetIgnoreGhostLevels(this->IgnoreGhostLevels);
    cellToPoint->AddVolumeArrayName(this->InputScalarsSelection);
    cellToPoint->Update();
    inputCopy->ShallowCopy(cellToPoint->GetOutput());
    cellToPoint->Delete();
    } 

  vtkTimerLog::MarkEndEvent("CellToPoint");

  tmp = vtkPolyData::New();

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = inputCopy->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    block = vtkImageData::New();
    inputCopy->GetBlock(blockId, block);
    this->ExecuteBlock(block, tmp);
    block->Delete();
    block = NULL;
    }
  
  // Copy appends to output (one part per output).
  output = this->GetOutput();
  output->ShallowCopy(tmp);
  tmp->Delete();
  tmp = 0;

  // Get rid of extra ghost levels.
  output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);

  // Get rid of the point array.
  output->GetPointData()->RemoveArray(this->InputScalarsSelection);

  inputCopy->Delete();
  inputCopy = NULL;
  this->DeleteInternalPipeline();
}

//-----------------------------------------------------------------------------
void vtkCTHAMRContour::ExecuteBlock(vtkImageData* block, 
                                    vtkPolyData* tmp)
{
  vtkFloatArray* pointVolumeFraction;
  const char* arrayName;
  int *dims;

  arrayName = this->InputScalarsSelection;
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
    if (cellVolumeFraction->GetDataType() == VTK_FLOAT ||
        cellVolumeFraction->GetDataType() == VTK_DOUBLE)
      {
      pointVolumeFraction = vtkFloatArray::New();
      pointVolumeFraction->SetNumberOfTuples(block->GetNumberOfPoints());
      this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                       pointVolumeFraction, 
                                       dims);        
      }
    else
      {
      vtkErrorMacro("Expecting volume fraction to be of type float.");
      return;
      }

    block->GetPointData()->AddArray(pointVolumeFraction);
    pointVolumeFraction->Delete();
    //block->GetCellData()->RemoveArray(arrayName);
    }

  // Get rid of ghost cells.
  //if ( ! this->IgnoreGhostLevels)
  //  { // In the future, let the cell to point filter remove the ghost levels.
  //  int ext[6];
  //  int extraGhostLevels = this->GetInput()->GetNumberOfGhostLevels() - this->GetOutput()->GetUpdateGhostLevel();
  //  block->GetExtent(ext);
  //  ext[0] += extraGhostLevels;
  //  ext[2] += extraGhostLevels;
  //  ext[4] += extraGhostLevels;
  //  ext[1] -= extraGhostLevels;
   // ext[3] -= extraGhostLevels;
  //  ext[5] -= extraGhostLevels;
  //  block->SetUpdateExtent(ext);
  //  block->Crop();
  //  }
  
  this->ExecutePart(this->InputScalarsSelection, block, tmp);
}


//------------------------------------------------------------------------------
void vtkCTHAMRContour::CreateInternalPipeline()
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
  this->Contour->ComputeNormalsOff();
#else
  this->Contour = vtkContourFilter::New();
#endif
  this->Contour->SetInput(this->Image);
    
  // Copy the contour values to the internal filter.
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();
  this->Contour->SetNumberOfContours(numContours);
  for (int i=0; i < numContours; i++)
    {
    this->Contour->SetValue(i,values[i]);
    }
         
  this->FinalAppend->AddInput(this->Contour->GetOutput());
  this->FinalAppend->AddInput(this->PolyData);
}

//------------------------------------------------------------------------------
void vtkCTHAMRContour::DeleteInternalPipeline()
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
  if (this->FinalAppend)
    {
    this->FinalAppend->Delete();
    this->FinalAppend = 0;
    }
  if (this->Contour)
    {
    this->Contour->Delete();
    this->Contour = 0;
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRContour::ExecutePart(const char* arrayName,
                                   vtkImageData* block, 
                                   vtkPolyData* appendCache)
{
  block->GetPointData()->SetActiveScalars(arrayName);

  this->Image->ShallowCopy(block);
  this->PolyData->ShallowCopy(appendCache);
  this->FinalAppend->Update();
  appendCache->ShallowCopy(this->FinalAppend->GetOutput());
}

template <class T>
void vtkExecuteCellDataToPointData(T* pCell,float* pPoint, int dims[3])
{
  int count;
  int i, j, k;
  int iEnd, jEnd, kEnd;
  int jInc, kInc;
  float* pPointStart = pPoint;

  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;
  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;

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
  pPoint = pPointStart;
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
        *pPoint = *pPoint / (T)(count);
        ++pPoint;
        }
      }
    }
}
//------------------------------------------------------------------------------
void vtkCTHAMRContour::ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, int *dims)
{
  float *pPoint;

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());
  if (cellVolumeFraction->GetDataType() == VTK_FLOAT)
    {
    pPoint = pointVolumeFraction->GetPointer(0);
    float* pCell = (float*)(cellVolumeFraction->GetVoidPointer(0));
    vtkExecuteCellDataToPointData(pCell,pPoint,dims);
    }
  else if (cellVolumeFraction->GetDataType() == VTK_DOUBLE)
    {
    pPoint = pointVolumeFraction->GetPointer(0);
    double* pCell = (double*)(cellVolumeFraction->GetVoidPointer(0));
    vtkExecuteCellDataToPointData(pCell,pPoint,dims);
    }
  else
    {
    vtkErrorMacro("Expecting double or float array.");
    }
}




// A bit harder to template.  Do it later.
//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point ,verage all cells touching the point.
void vtkCTHAMRContour::ExecuteCellDataToPointData2(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, vtkCTHData* data)
{
  int x, y, z, maxX, maxY, maxZ;
  float *pPoint0;
  float *pPoint;
  double *pCell0;
  double *pCell;
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
  // Lets ignore ghost levels.
  // For parallel runs, we might keep the ghost levels,
  // or we could transmit neightboring block values.
  int numGhostLevels = data->GetNumberOfGhostLevels();
  int coreMinX, coreMinY, coreMinZ;
  int coreMaxX, coreMaxY, coreMaxZ;

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

  // Lets ignore ghost levels.
  // This assumes ghost levels are uniform around each grid.
  coreMinX = coreMinY = coreMinZ = numGhostLevels;
  coreMaxX = maxX - numGhostLevels;
  coreMaxY = maxY - numGhostLevels;
  coreMaxZ = maxZ - numGhostLevels;

  // It might be faster to have a separate loop for interior points.
  // Loop over all points.
  pPoint0 = pointVolumeFraction->GetPointer(0);
  pCell0 = (double*)(cellVolumeFraction->GetVoidPointer(0));
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
              // Condition to ignore ghost levels. (Added as after thought).
              if (z >= coreMinZ && z <= coreMaxZ && y >= coreMinY && 
                  y <= coreMaxY && x >= coreMinX && x <= coreMaxX)
                {
                if (z == coreMinZ || y == coreMinY || x == coreMinZ)
                  {
                  *pPoint = this->ComputeSharedPoint(blockId, blockList, 
                                                    x, y, z, pCell0, pPoint0, data);
                  ++pPoint;
                  // Do not increment the cell pointer for negative boundary faces.
                  }
                else if (z == coreMaxZ || y == coreMaxY || x == coreMaxX)
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
                } // Pixel is not a ghost level.
              else
                {  
                *pPoint = 0.0;
                ++pPoint;
                // The Min boundary handles difference between dim points and cells.
                // Increment like normal here.
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
float vtkCTHAMRContour::ComputeSharedPoint(int blockId, vtkIdList* blockList,
                                           int x, int y, int z, 
                                           double* pCell, float* pPoint, 
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
  int cIncY = dims[0]-1;
  int cIncZ = (dims[1]-1)*cIncY;
  int ghost = input->GetNumberOfGhostLevels();
  int pMinX = ghost;
  int pMinY = ghost;
  int pMinZ = ghost;
  int pMaxX = dims[0]-1 - ghost;
  int pMaxY = dims[1]-1 - ghost;
  int pMaxZ = dims[2]-1 - ghost;

  // First add cells local to block.
  spacing = input->GetBlockSpacing(blockId);
  // Assume x, y and z spacing are the same.
  weight = 1.0/spacing[0];
  id = (blockId * numCellsPerBlock) + x + y*cIncY + z*cIncZ;
  if (x > pMinX && y > pMinY && z > pMinZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncY - cIncZ];
    }
  if (x < pMaxX && y > pMinY && z > pMinZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncY - cIncZ];
    }
  if (x > pMinX && y < pMaxY && z > pMinZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncZ];
    }
  if (x < pMaxX && y < pMaxY && z > pMinZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncZ];
    }
  if (x > pMinX && y > pMinY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1 - cIncY];
    }
  if (x < pMaxX && y > pMinY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - cIncY];
    }
  if (x > pMinX && y < pMaxY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id - 1];
    }
  if (x < pMaxX && y < pMaxY && z < pMaxZ)
    {
    sumWeight += weight;
    sum += weight*pCell[id];
    }

  // Next find all the blocks that share the point.
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
        if (x>pMinX && x<pMaxX && y>pMinY && y<pMaxY && z>pMinZ && z<pMaxZ)
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
        if (dx < epsilon && x > pMinX) {--x0;}
        if (dy < epsilon && y > pMinY) {--y0;}
        if (dz < epsilon && z > pMinZ) {--z0;}
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


//------------------------------------------------------------------------------
// Note, this only works with no ghost levels.
void vtkCTHAMRContour::FindBlockNeighbors(vtkCTHData* self, int blockId, vtkIdList* blockList)
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
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkCTHAMRContour::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i,value);
}

//------------------------------------------------------------------------------
// Get the ith contour value.
double vtkCTHAMRContour::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//------------------------------------------------------------------------------
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double *vtkCTHAMRContour::GetValues()
{
  return this->ContourValues->GetValues();
}

//------------------------------------------------------------------------------
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void vtkCTHAMRContour::GetValues(double *contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//------------------------------------------------------------------------------
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void vtkCTHAMRContour::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//------------------------------------------------------------------------------
// Get the number of contours in the list of contour values.
int vtkCTHAMRContour::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//------------------------------------------------------------------------------
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkCTHAMRContour::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

//------------------------------------------------------------------------------
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkCTHAMRContour::GenerateValues(int numContours, double
                                             rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

//------------------------------------------------------------------------------
void vtkCTHAMRContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());
  os << indent << "IgnoreGhostLevels: " << this->IgnoreGhostLevels << endl;

  if (this->InputScalarsSelection)
    {
    os << indent << "InputScalarsSelection: " 
       << this->InputScalarsSelection << endl;
    }
}




