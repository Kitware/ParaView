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
#include "vtkTimerLog.h"



vtkCxxRevisionMacro(vtkCTHExtractAMRPart, "1.1");
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
}

//----------------------------------------------------------------------------
vtkCTHExtractAMRPart::~vtkCTHExtractAMRPart()
{
  this->VolumeArrayNames->Delete();
  this->VolumeArrayNames = NULL;
  this->SetClipPlane(NULL);
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
  vtkPolyData* output;
  vtkImageData* block; 
  vtkAppendPolyData** appends;
  int idx, num;

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
  numBlocks = input->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    block = vtkImageData::New();
    input->GetBlock(blockId, block);
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
}

//-----------------------------------------------------------------------------
void vtkCTHExtractAMRPart::ExecuteBlock(vtkImageData* block, 
                                        vtkAppendPolyData** appends)
{
  int idx, num;
  const char* arrayName;
  int *dims;

  // Loop over parts to convert volume fractions to point arrays.
  num = this->VolumeArrayNames->GetNumberOfStrings();
  for (idx = 0; idx < num; ++idx)
    {
    vtkFloatArray* pointVolumeFraction;
    vtkDataArray* cellVolumeFraction;
    dims = block->GetDimensions();
    arrayName = this->VolumeArrayNames->GetString(idx);

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




