/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRCellToPointData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRCellToPointData.h"
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
#include "vtkStringList.h"
#include "vtkPlane.h"
#include "vtkStringList.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"



vtkCxxRevisionMacro(vtkCTHAMRCellToPointData, "1.4");
vtkStandardNewMacro(vtkCTHAMRCellToPointData);

//----------------------------------------------------------------------------
vtkCTHAMRCellToPointData::vtkCTHAMRCellToPointData()
{
  this->VolumeArrayNames = vtkStringList::New();
  
  // So we do not have to keep creating idList in a loop of Execute.
  this->IdList = vtkIdList::New();

  this->IgnoreGhostLevels = 0;
}

//----------------------------------------------------------------------------
vtkCTHAMRCellToPointData::~vtkCTHAMRCellToPointData()
{
  this->VolumeArrayNames->Delete();
  this->VolumeArrayNames = 0;

  this->IdList->Delete();
  this->IdList = NULL;
}

//--------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::RemoveAllVolumeArrayNames()
{
  this->VolumeArrayNames->RemoveAllItems();  
}

//--------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::AddVolumeArrayName(const char* arrayName)
{
  this->VolumeArrayNames->AddString(arrayName);
}

//--------------------------------------------------------------------------
int vtkCTHAMRCellToPointData::GetNumberOfVolumeArrayNames()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}

//--------------------------------------------------------------------------
const char* vtkCTHAMRCellToPointData::GetVolumeArrayName(int idx)
{
  return this->VolumeArrayNames->GetString(idx);
}

//----------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::Execute()
{
  vtkCTHData* input = this->GetInput();
  vtkCTHData* output = this->GetOutput();

  this->CreateOutputGeometry(input, output);
  this->UpdateProgress(.2);

  // If there are no ghost cells, then try our fancy way of
  // computing cell volume fractions.  It finds all point cells
  // including cells from neighboring blocks that touch the point.
  if (input->GetNumberOfGhostLevels() == 0 || this->IgnoreGhostLevels)
    {
    this->ExecuteCellDataToPointData2(input, output);
    }
  else
    { // Use ghost levels.
    int numArrayNames;
    int arrayNamesIdx;
    const char* arrayName;
    numArrayNames = this->VolumeArrayNames->GetNumberOfStrings();
    for (arrayNamesIdx = 0; arrayNamesIdx < numArrayNames; ++arrayNamesIdx)
      {
      this->UpdateProgress(.2 + .6 * static_cast<double>(arrayNamesIdx)/static_cast<double>(numArrayNames));
      arrayName = this->VolumeArrayNames->GetString(arrayNamesIdx);
      vtkDataArray* cellVolumeFraction;
      vtkFloatArray* pointVolumeFraction;

      cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
      if (cellVolumeFraction == NULL)
        {
        vtkErrorMacro("Could not find cell array " << arrayName);
        return;
        }
      if (cellVolumeFraction->GetDataType() == VTK_FLOAT ||
          cellVolumeFraction->GetDataType() == VTK_DOUBLE)
        {
        pointVolumeFraction = vtkFloatArray::New();
        pointVolumeFraction->SetNumberOfTuples(output->GetNumberOfPoints());
        this->ExecuteCellDataToPointData(input, cellVolumeFraction, 
                                         output, pointVolumeFraction);
        output->GetPointData()->AddArray(pointVolumeFraction);
        pointVolumeFraction->Delete();                                             
        }
      else
        {
        vtkErrorMacro("Expecting volume fraction to be of type double or float.");
        return;
        }
      }
    }
  this->UpdateProgress(.8);
  this->CopyCellData(input, output);
}

//------------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::CreateOutputGeometry(vtkCTHData* input,
                                                    vtkCTHData* output)
{
  int numGhostLevels;
  
  numGhostLevels = input->GetNumberOfGhostLevels();
  if (numGhostLevels == 0)
    {
    output->CopyStructure(input);
    return;
    }
    
  int block, numBlocks, level;
  int ext[6];
  numBlocks = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);
  output->SetTopLevelSpacing(input->GetTopLevelSpacing());
  output->SetTopLevelOrigin(input->GetTopLevelOrigin());
  for (block = 0; block < numBlocks; ++block)
    {
    level = input->GetBlockLevel(block);
    input->GetBlockCellExtent(block,ext);
    if (ext[1] > ext[0])
      {
      ext[0] += numGhostLevels;
      ext[1] -= numGhostLevels;
      }
    if (ext[3] > ext[2])
      {
      ext[2] += numGhostLevels;
      ext[3] -= numGhostLevels;
      }
    if (ext[5] > ext[4])
      {
      ext[4] += numGhostLevels;
      ext[5] -= numGhostLevels;
      }
    output->SetBlockCellExtent(block, level, ext);
}

//----------------------------------------------------------------------------
// This assumes that ghost levels exist.  No boundary condition checks.
template <class T>
void vtkCTHAMRExecuteCellDataToPointData(T* pCell,float* pPoint, 
                                         int cDims[3], int pDims[3])
{
  int i, j, k;
  int jInc, kInc;

  // Increments are for the cell array.
  // This assumes one level of ghost cells on input.
  // Cell dims is pDims+1;
  jInc = cDims[0];
  kInc = (cDims[1]) * jInc;

  // Loop through the points.
  for (k = 0; k < pDims[2]; ++k)
    {
    for (j = 0; j < pDims[1]; ++j)
      {
      for (i = 0; i < pDims[0]; ++i)
        {        
        // Add cell value to all points of cell.
        *pPoint = pCell[0];
        *pPoint += pCell[1];
        *pPoint += pCell[jInc];
        *pPoint += pCell[jInc+1];
        if (cDims[2] > 1)
          {
          *pPoint += pCell[kInc];
          *pPoint += pCell[kInc+1];
          *pPoint += pCell[kInc+jInc];
          *pPoint += pCell[kInc+jInc+1];
          // Average 8 neighbors.
          *pPoint = *pPoint * 0.125;
          }
        else
          {
          // Average 4 neighbors.
          *pPoint = *pPoint * 0.25;
          }    
        // Increment pointers
        ++pPoint;
        ++pCell;
        }
      // Skip over last cell to the start of the next row.
      ++pCell;
      }
    // Skip over the last row to the start of the next plane.
    pCell += jInc;
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::ExecuteCellDataToPointData(
                         vtkCTHData* input, vtkDataArray *cellVolumeFraction, 
                         vtkCTHData* output, vtkFloatArray *pointVolumeFraction)
{
  vtkIdType inStart, outStart;
  int block, numBlocks;
  float *pPoint;
  int* pDims;
  int* cDims;

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());

  inStart = outStart = 0;
  numBlocks = input->GetNumberOfBlocks();
  for (block = 0; block < numBlocks; ++block)
    {
    pDims = output->GetBlockPointDimensions(block); 
    cDims = input->GetBlockCellDimensions(block); 
    pPoint = pointVolumeFraction->GetPointer(outStart);
    if (cellVolumeFraction->GetDataType() == VTK_FLOAT)
      {
      float* pCell = (float*)(cellVolumeFraction->GetVoidPointer(inStart));
      vtkCTHAMRExecuteCellDataToPointData(pCell,pPoint,cDims,pDims);
      }
    else if (cellVolumeFraction->GetDataType() == VTK_DOUBLE)
      {
      double* pCell = (double*)(cellVolumeFraction->GetVoidPointer(inStart));
      vtkCTHAMRExecuteCellDataToPointData(pCell,pPoint,cDims,pDims);
      }
    else
      {
      vtkErrorMacro("Expecting double or float array.");
      }
    inStart += (cDims[0])*(cDims[1])*(cDims[2]);
    // This assumes one level of ghost cells on input.
    outStart += (pDims[0])*(pDims[1])*(pDims[2]);
    }
}




// A bit harder to template.  Do it later.
//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point ,verage all cells touching the point.
void vtkCTHAMRCellToPointData::ExecuteCellDataToPointData2(
                          vtkCTHData* input, vtkCTHData* output)
{
  int x, y, z;
  float *pPoint0;
  float *pPoint;
  double *pCell0;
  double *pCellX, *pCellY, *pCellZ;
  int cInc[6];
  int pIncX, pIncY, pIncZ;
  int cIncX, cIncY, cIncZ;
  int blockId, numBlocks;
  vtkIdList* blockList = vtkIdList::New();
  int cDims[3];
  int pDims[3];
  int pMaxX, pMaxY, pMaxZ;
  vtkDataArray *cellVolumeFraction;
  vtkDataArray *pointVolumeFraction;
    
  input->GetBlockCellDimensions(0, cDims);
  output->GetBlockPointDimensions(0, pDims);

  pIncX = 1;
  pIncY = pDims[0];
  pIncZ = pIncY * pDims[1];
  
  cIncX = 1;
  cIncY = cDims[0];
  cIncZ = cIncY * cDims[1];
  
  int numCellsPerBlock = input->GetNumberOfCellsPerBlock();
  int numPtsPerBlock = output->GetNumberOfPointsPerBlock();
  // Ignore ghost levels.
  // For parallel runs, we might keep the ghost levels,
  // or we could transmit neightboring block values.
  int numGhostLevels = input->GetNumberOfGhostLevels();

  // Precompute for speed.  We have to do alot of comparisons
  // For boundary condition checking.
  pMaxX = pDims[0]-1;
  pMaxY = pDims[1]-1;
  pMaxZ = pDims[2]-1;

  // All neighbor cell incs except 0 and 1.
  // Funny order is for cache locality.
  cInc[0] = cDims[0];              // +y 
  cInc[1] = 1 + cInc[0];              // +x +y
  cInc[2] = (cDims[1]) * (cDims[0]);// +z
  cInc[3] = 1 + cInc[2];              // +x +z
  cInc[4] = cInc[0] + cInc[2];        // +y +z
  cInc[5] = 1 + cInc[0] + cInc[2];    // +x +y +z


  // Loop over all of the selected arrays.
  // This would be more efficient if we kept an array of pointers
  // and has this loop as the inner loop.  We would only compute 
  // neighbors once for each block.
  // Tried this.  It was a real mess.
  int numArrayNames = this->VolumeArrayNames->GetNumberOfStrings();
  int arrayNamesIdx;
  const char* arrayName;

  // Create all of the point arrays.
  for (arrayNamesIdx = 0; arrayNamesIdx < numArrayNames; ++arrayNamesIdx)
    {
    arrayName = this->VolumeArrayNames->GetString(arrayNamesIdx);
    cellVolumeFraction = input->GetCellData()->GetScalars(arrayName);
    if (cellVolumeFraction == NULL)
      {
      vtkErrorMacro("Could not find cell array " << arrayName);
      return;
      }
    if (cellVolumeFraction->GetDataType() != VTK_DOUBLE)
      {
      vtkErrorMacro("Expecting volume fraction to be of type double.");
      return;
      }
    pointVolumeFraction = vtkFloatArray::New();
    pointVolumeFraction->SetName(arrayName);
    pointVolumeFraction->SetNumberOfTuples(output->GetNumberOfPoints());
    output->GetPointData()->AddArray(pointVolumeFraction);
    pointVolumeFraction->Delete();
    }


  // Now lets loop over blocks. Low levels first.
  // Processing the low levels first will allow me to force some
  // boundary points (that are not on neighbor grid points) to obey
  // linear interpolation of neighbor's face.
  numBlocks = input->GetNumberOfBlocks();
  int blocksFinished = 0;
  int level = 0;
  while (blocksFinished < numBlocks)
    { // Look for all blocks with level.
    this->UpdateProgress(.2 + .6 * static_cast<double>(blocksFinished)/static_cast<double>(numBlocks));
    for (blockId = 0; blockId < numBlocks; ++blockId)
      {
      // We should really have level a better part of the data set structure.
      // Look to level array for now.
      if (input->GetBlockLevel(blockId) < 0)
        {
        vtkErrorMacro("Bad block level");
        // avoids an infinite loop.
        ++blocksFinished;
        }
      if (input->GetBlockLevel(blockId) == level)
        {
        // Mark a block as finished (before we start processing the block).
        ++blocksFinished;
        // Make a list of neighbors so each point does not need to look through
        // all the blocks.  With a better data structure, we might have
        // a method to return neighbors of indexed by block face.
        this->FindBlockNeighbors(output, blockId, blockList);

        // Best place I can find for this loop.  After block neighbor ..
        // Any later is a real mess.
        for (arrayNamesIdx = 0; arrayNamesIdx < numArrayNames; ++arrayNamesIdx)
          {
          arrayName = this->VolumeArrayNames->GetString(arrayNamesIdx);
          cellVolumeFraction = input->GetCellData()->GetScalars(arrayName);
          pointVolumeFraction = output->GetPointData()->GetScalars(arrayName);

          // It might be faster to have a separate loop for interior points.
          // Loop over all points.
          pPoint0 = (float*) pointVolumeFraction->GetVoidPointer(0);
          pCell0 = (double*)(cellVolumeFraction->GetVoidPointer(0));

          // Move pointers to start of block.
          pPoint = pPoint0 + (blockId*numPtsPerBlock);
          // Skip to correct block
          pCellZ = pCell0 + (blockId*numCellsPerBlock);
          // Skip over initial ghost cells.
          if (cDims[2] > 1)
            {
            pCellZ += numGhostLevels * (cIncX+cIncY+cIncZ);
            }
          else
            { // 2D case.
            pCellZ += numGhostLevels * (cIncX+cIncY);
            }
          // Loop over all points in this output block.
          // March through cells as if there were no ghost cells.
          for (z = 0; z <= pMaxZ; ++z)
            {
            pCellY = pCellZ;
            for (y = 0; y <= pMaxY; ++y)
              {
              pCellX = pCellY;
              for (x = 0; x <= pMaxX; ++x)
                {
                if (z == 0 || y == 0 || x == 0)
                  {
                  *pPoint = this->ComputeSharedPoint(blockId, blockList, 
                                                    x, y, z, pCell0, pPoint0, input, output);
                  ++pPoint;
                  // Do not increment the cell pointer for negative boundary faces.
                  }
                else if (z == pMaxZ || y == pMaxY || x == pMaxX)
                  {
                  *pPoint = this->ComputeSharedPoint(blockId, blockList, 
                                                    x, y, z, pCell0, pPoint0, input, output);
                  ++pPoint;
                  }
                else
                  {
                  // This fast path for interior point should speed things up.
                  // Average the eight neighboring cells.
                  if (cDims[2] > 1)
                    {
                    *pPoint = (pCellX[0] + pCellX[1] + pCellX[cInc[0]] + pCellX[cInc[1]]
                            + pCellX[cInc[2]] + pCellX[cInc[3]] + pCellX[cInc[4]]
                            + pCellX[cInc[5]]) * 0.125;
                    }
                  else
                    { // 2D case
                    *pPoint = (pCellX[0] + pCellX[1] + pCellX[cInc[0]] + pCellX[cInc[1]]) * 0.25;
                    }
                  ++pPoint;
                  }
                // March through the cells like there are no ghost cells.
                // first two point use the first cell.
                if (x > 0)
                  {
                  ++pCellX;
                  }
                } // x loop
              if (y > 0)
                {
                pCellY += cIncY;
                }  
              } // y loop
            if (z > 0)
              {
              pCellZ += cIncZ;  
              }
            } // z loop
          } // array name loop
        } // if (data->GetBlockLevel(blockId) == level)
      } // for (blockId = 0; blockId < numBlocks; ++blockId)
      // Move to next level.
      ++level;
    } // while (blocksFinished < numBlocks)
  blockList->Delete();
}



// I am converting to skip over ghost cells by:
// Changing only increments.

//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point, average all cells touching the point.
// Weight the cell by inverse of spacing.
// We need the point array only for linear interpolation constraint.
float vtkCTHAMRCellToPointData::ComputeSharedPoint(int blockId, vtkIdList* blockList,
                                               int x, int y, int z, 
                                               double* pCell, float* pPoint, 
                                               vtkCTHData* input, vtkCTHData* output)
{
  int pDims[3];
  int cDims[3];
  input->GetBlockCellDimensions(0,cDims);  
  output->GetBlockPointDimensions(0,pDims);
  int ghost = input->GetNumberOfGhostLevels();
  // Increments:
  int cIncY = cDims[0];
  int cIncZ = (cDims[1])*cIncY;
  int pIncY = pDims[0];
  int pIncZ = (pDims[1])*pIncY;
  
  // Now hide the ghost cells.
  if (cDims[2] > 1)
    {
    pCell += ghost*(1+cIncY+cIncZ);
    cDims[2] = cDims[2] - (2*ghost);
    }
  else
    { // 2D case
    pCell += ghost*(1+cIncY);
    }
  cDims[0] = cDims[0] - (2*ghost);
  cDims[1] = cDims[1] - (2*ghost);
  
  // Every thing else should be the same.  
  double* spacing;
  double sum = 0.0;
  double weight;
  double sumWeight = 0.0;
  double epsilon;
  double origin[3];
  double outside[3];
  int numPtsPerBlock = output->GetNumberOfPointsPerBlock();
  int numCellsPerBlock = input->GetNumberOfCellsPerBlock();
  int i, id, num;
  int x0, x1, y0, y1, z0, z1;
  double dx, dy, dz;
  double pt[3];
  int pMaxX = pDims[0]-1;
  int pMaxY = pDims[1]-1;
  int pMaxZ = pDims[2]-1;
  if (pDims[2] == 1)
    { // Add one so that 4 are averaged.
    ++pMaxZ;
    }
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

  // Next find all the blocks that share the point.
  // Compute point in world space.
  input->GetBlockOrigin(blockId, origin);
  // --- Here we do need to skip ghost cells.
  origin[0] = origin[0] + ghost*spacing[0];
  origin[1] = origin[1] + ghost*spacing[1];
  origin[2] = origin[2] + ghost*spacing[2];
  //---
  pt[0] = origin[0] + (double)x * spacing[0];
  pt[1] = origin[1] + (double)y * spacing[1];
  pt[2] = origin[2] + (double)z * spacing[2];
  epsilon = spacing[0] / 1000.0;

  num = blockList->GetNumberOfIds();
  for (i = 0; i < num; ++i)
    {
    id = blockList->GetId(i);
    spacing = input->GetBlockSpacing(id);
    input->GetBlockOrigin(id, origin);
    // --- Here we do need to skip ghost cells.
    origin[0] = origin[0] + ghost*spacing[0];
    origin[1] = origin[1] + ghost*spacing[1];
    origin[2] = origin[2] + ghost*spacing[2];
    //---
    if (pt[0] > origin[0]-epsilon && pt[1] > origin[1]-epsilon &&
        pt[2] > origin[2]-epsilon)
      {
      weight = 1.0/spacing[0];
      outside[0] = origin[0] + spacing[0]*(double)(pDims[0]-1);
      outside[1] = origin[1] + spacing[1]*(double)(pDims[1]-1);
      outside[2] = origin[2] + spacing[2]*(double)(pDims[2]-1);
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
          float* cornerPoint = pPoint + id*numPtsPerBlock + x + y*pIncY + z*pIncZ;
          // Since there are only six cases (3 edges and 3 faces),
          // just have a condition for each.
          if (dx > epsilon && dy <= epsilon && dz <= epsilon)
            { // x edge
            return 0.5 * (cornerPoint[0] + cornerPoint[1]);
            }
          else if (dx <= epsilon && dy > epsilon && dz <= epsilon)
            { // y edge
            return 0.5 * (cornerPoint[0] + cornerPoint[pIncY]);
            }
          else if (dx <= epsilon && dy <= epsilon && dz > epsilon)
            { // z edge
            return 0.5 * (cornerPoint[0] + cornerPoint[pIncZ]);
            }
          else if (dx > epsilon && dy > epsilon && dz <= epsilon)
            { // xy face
            return 0.25 * (cornerPoint[0] + cornerPoint[1] + 
                           cornerPoint[pIncY] + cornerPoint[1+pIncY]);
            }
          else if (dx > epsilon && dy <= epsilon && dz > epsilon)
            { // xz face
            return 0.25 * (cornerPoint[0] + cornerPoint[1] + 
                           cornerPoint[pIncZ] + cornerPoint[1+pIncZ]);
            }
          else if (dx <= epsilon && dy > epsilon && dz > epsilon)
            { // yz face
            return 0.25 * (cornerPoint[0] + cornerPoint[pIncY] + 
                           cornerPoint[pIncZ] + cornerPoint[pIncY+pIncZ]);
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

//------------------------------------------------------------------------------
// Note, this only works with no ghost levels.
void vtkCTHAMRCellToPointData::FindBlockNeighbors(vtkCTHData* self, int blockId, vtkIdList* blockList)
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
// Sort of stupid, but dims is the point dimesnions of input.
// This assumes that ghost levels exist.  No boundary condition checks.
template <class T>
void vtkCTHAMRCellDataToPointDataClipArrays(T* pIn, int *inDims, 
                                            T* pOut, int *outDims, 
                                            int numBlocks, int numGhostLevels)
{
  int i, x, y, z;
  T *pInX, *pInY, *pInZ;
  int inIncY, inIncZ, inIncBlock;
  
  inIncY = inDims[0];
  inIncZ = inDims[1]*inIncY;
  inIncBlock = inDims[2]*inIncZ;
  
  // It is easiest to skip over ghost cells at begining because
  // ghost levels are the same for all blocks.
  // Collapsed dimensions do not have ghost levels.
  if (outDims[0] > 1)
    {
    pIn += numGhostLevels;
    }
  if (outDims[1] > 1)
    {
    pIn += numGhostLevels*inIncY;
    }
  if (outDims[2] > 1)
    {
    pIn += numGhostLevels*inIncZ;
    }
  for (i = 0; i < numBlocks; ++i)
    {
    pInZ = pIn;
    for (z = 0; z < outDims[2]; ++z)
      {
      pInY = pInZ;
      for (y = 0; y < outDims[1]; ++y)
        {
        pInX = pInY;
        for (x = 0; x < outDims[0]; ++x)
          {
          *pOut++ = *pInX++;
          }
        pInY += inIncY;
        }
      pInZ += inIncZ;
      }
    pIn += inIncBlock;
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::CopyCellData(vtkCTHData* input, vtkCTHData* output)
{
  if (input->GetNumberOfGhostLevels() == 0)
    {
    output->GetCellData()->PassData(input->GetPointData());
    return;
    }
  int inDims[3];
  int outDims[3];
  int numBlocks = input->GetNumberOfBlocks();
  int numGhostLevels = input->GetNumberOfGhostLevels();
  int numArrays, i;
  vtkDataArray *inArray, *outArray;
  numArrays = input->GetCellData()->GetNumberOfArrays();
  input->GetBlockCellDimensions(0, inDims);
  output->GetBlockCellDimensions(0, outDims);
  
  for (i = 0; i < numArrays; ++i)
    {
    this->UpdateProgress(.8 + .2 * static_cast<double>(i)/static_cast<double>(numArrays));
    inArray = input->GetCellData()->GetArray(i);
    outArray = inArray->NewInstance();
    outArray->SetName(inArray->GetName());
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(output->GetNumberOfCells());
    void* inPtr = inArray->GetVoidPointer(0);
    void* outPtr = outArray->GetVoidPointer(0);
    switch (inArray->GetDataType())
      {
      vtkTemplateMacro6(vtkCTHAMRCellDataToPointDataClipArrays,
                        (VTK_TT *)(inPtr), inDims, (VTK_TT *)(outPtr),
                        outDims, numBlocks, numGhostLevels);
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
        outArray->Delete();
        return;
      }
    output->GetCellData()->AddArray(outArray);
    outArray->Delete();
    }
}



//------------------------------------------------------------------------------
void vtkCTHAMRCellToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "IgnoreGhostLevels: " << this->IgnoreGhostLevels << endl;
}




