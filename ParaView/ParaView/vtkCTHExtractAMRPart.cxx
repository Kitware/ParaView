/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHExtractAMRPart.cxx
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
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

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
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkStringList.h"
#include "vtkPlane.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"



vtkCxxRevisionMacro(vtkCTHExtractAMRPart, "1.2");
vtkStandardNewMacro(vtkCTHExtractAMRPart);
vtkCxxSetObjectMacro(vtkCTHExtractAMRPart,ClipPlane,vtkPlane);

//----------------------------------------------------------------------------
vtkCTHExtractAMRPart::vtkCTHExtractAMRPart()
{
  this->VolumeArrayNames = vtkStringList::New();
  this->Clipping = 0;
  this->ClipPlane = vtkPlane::New();
  // For consistent references.
  this->ClipPlane->Register(this);
  this->ClipPlane->Delete();

  // So we do not have to keep creating idList in a loop of Execute.
  this->IdList = vtkIdList::New();
}

//----------------------------------------------------------------------------
vtkCTHExtractAMRPart::~vtkCTHExtractAMRPart()
{
  this->VolumeArrayNames->Delete();
  this->VolumeArrayNames = NULL;
  this->SetClipPlane(NULL);

  this->IdList->Delete();
  this->IdList = NULL;
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
  vtkAppendPolyData** appends;
  int idx, num;

  inputCopy->ShallowCopy(input);

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

  // Create an append for each part (one part per output).
  num = this->VolumeArrayNames->GetNumberOfStrings();
  appends = new vtkAppendPolyData* [num];
  for (idx = 0; idx < num; ++idx)
    {
    appends[idx] = vtkAppendPolyData::New();  
    }

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = inputCopy->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    block = vtkImageData::New();
    inputCopy->GetBlock(blockId, block);
    this->ExecuteBlock(block, appends);
    block->Delete();
    block = NULL;
    }
  
  // Copy appends to output (one part per output).
  for (idx = 0; idx < num; ++idx)
    {
    output = this->GetOutput(idx);
    appends[idx]->Update();
    output->ShallowCopy(appends[idx]->GetOutput());
    appends[idx]->Delete();
    appends[idx] = NULL;

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
  delete [] appends;
  appends = NULL;
  inputCopy->Delete();
  inputCopy = NULL;
}

//-----------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecuteBlock(vtkImageData* block, 
                                        vtkAppendPolyData** appends)
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
    this->ExecutePart(arrayName, block, appends[idx]);
    } 
}


//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecutePart(const char* arrayName,
                                       vtkImageData* block, 
                                       vtkAppendPolyData* append)
{
  vtkPolyData* tmp;
  vtkClipPolyData *clip0;
  vtkDataSetSurfaceFilter *surface;
  vtkAppendPolyData *append1;
  vtkAppendPolyData *append2 = NULL;

  vtkTimerLog::MarkStartEvent("Execute Part");

  block->GetPointData()->SetActiveScalars(arrayName);

  // Create the contour surface.
#ifdef VTK_USE_PATENTED
  //vtkContourFilter *contour = vtkContourFilter::New();
  vtkContourFilter *contour = vtkKitwareContourFilter::New();
  // vtkDataSetSurfaceFilter does not generate normals, so they will be lost.
  contour->ComputeNormalsOff();
#else
  vtkContourFilter *contour = vtkContourFilter::New();
#endif
  contour->SetInput(block);
  contour->SetValue(0, 0.5);
  //contour->SelectInputScalars(arrayName);

  vtkTimerLog::MarkStartEvent("CTH Contour");
  contour->Update();
  vtkTimerLog::MarkEndEvent("CTH Contour");

  // Create the capping surface for the contour and append.
  append1 = vtkAppendPolyData::New();
  append1->AddInput(contour->GetOutput());
  surface = vtkDataSetSurfaceFilter::New();
  surface->SetInput(block);
  tmp = surface->GetOutput();

  vtkTimerLog::MarkStartEvent("Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("surface");

  // Clip surface less than volume fraction 0.5.
  clip0 = vtkClipPolyData::New();
  clip0->SetInput(surface->GetOutput());
  clip0->SetValue(0.5);
  //clip0->SelectInputScalars(arrayName);
  tmp = clip0->GetOutput();
  vtkTimerLog::MarkStartEvent("Clip Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("Clip Surface");
  append1->AddInput(clip0->GetOutput());

  vtkTimerLog::MarkStartEvent("Append");
  append1->Update();
  vtkTimerLog::MarkEndEvent("Append");

  tmp = append1->GetOutput();
  
  if (this->Clipping && this->ClipPlane)
    {
    vtkClipPolyData *clip1, *clip2;
    // We need to append iso and capped surfaces.
    append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    clip1 = vtkClipPolyData::New();
    clip1->SetInput(tmp);
    clip1->SetClipFunction(this->ClipPlane);
    append2->AddInput(clip1->GetOutput());
    // We need to create a capping surface.
#ifdef VTK_USE_PATENTED
    vtkCutter *cut = vtkKitwareCutter::New();
#else
    vtkCutter *cut = vtkCutter::New();
#endif
    cut->SetInput(block);
    cut->SetCutFunction(this->ClipPlane);
    cut->SetValue(0, 0.0);
    clip2 = vtkClipPolyData::New();
    clip2->SetInput(cut->GetOutput());
    clip2->SetValue(0.5);
    //clip2->SelectInputScalars(arrayName);
    append2->AddInput(clip2->GetOutput());
    append2->Update();
    tmp = append2->GetOutput();
    clip1->Delete();
    clip1 = NULL;
    cut->Delete();
    cut = NULL;
    clip2->Delete();
    clip2 = NULL;
    }

  append->AddInput(tmp);

  contour->Delete();
  surface->Delete();
  clip0->Delete();
  // Make sure all temperary fitlers actually delete.
  append1->SetOutput(NULL);
  append1->Delete();
  if (append2)
    {
    // Make sure all temperary fitlers actually delete.
    append2->SetOutput(NULL);
    append2->Delete();
    }

  vtkTimerLog::MarkEndEvent("Execute Part");
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
  float *pPoint;
  float *pCell0;
  float *pCell;
  int cInc[6];
  int pIncX, pIncY, pIncZ;
  int blockId, numBlocks;

  int *dims = data->GetDimensions();
  pIncX = 1;
  pIncY = dims[0];
  pIncZ = pIncY * dims[1];

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
  pPoint = pointVolumeFraction->GetPointer(0);
  pCell0 = (float*)(cellVolumeFraction->GetVoidPointer(0));
  pCell = pCell0;
  numBlocks = data->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    // Loop all points.
    for (z = 0; z <= maxZ; ++z)
      {
      for (y = 0; y <= maxY; ++y)
        {
        for (x = 0; x <= maxX; ++x)
          {
          if (z == 0 || y == 0 || x == 0)
            {
            *pPoint = this->ComputeSharedPoint(blockId, x, y, z, pCell0, data);
            ++pPoint;
            // Do not increment the cell pointer for negative boundary faces.
            }
          else if (z == maxZ || y == maxY || x == maxX)
            {
            *pPoint = this->ComputeSharedPoint(blockId, x, y, z, pCell0, data);
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
          }
        }
      }    
    }

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());
}



//------------------------------------------------------------------------------
// I am trying a better way of converting cell data to point data.
// This should olny be used when there are no ghost cells.
// For each point ,verage all cells touching the point.
float vtkCTHExtractAMRPart::ComputeSharedPoint(int blockId, int x, int y, int z, 
                                                float* pCell, vtkCTHData* input)
{
  int* dims = input->GetDimensions();
  vtkIdType id, num;
  float sum = 0.0;

  id = x + dims[0]*(y + dims[1]*(z + blockId*dims[2]));
  this->FindPointCells(input, id, this->IdList);

  //  average cells values.
  num = this->IdList->GetNumberOfIds();
  for (id = 0; id < num; ++id)
    {
    sum += pCell[this->IdList->GetId(id)];
    }

  return sum / (float)(num); 
}

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
  int* dims = self->GetDimensions();
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
















//------------------------------------------------------------------------------
void vtkCTHExtractAMRPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeArrayNames: \n";
  vtkIndent i2 = indent.GetNextIndent();
  this->VolumeArrayNames->PrintSelf(os, i2);
  if (this->Clipping)
    {
    os << indent << "Clipping: On\n";
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
  else  
    {
    os << indent << "Clipping: Off\n";
    }
}




