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
#include "vtkCTHAMRCellToPointData.h"
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



vtkCxxRevisionMacro(vtkCTHExtractAMRPart, "1.13");
vtkStandardNewMacro(vtkCTHExtractAMRPart);
vtkCxxSetObjectMacro(vtkCTHExtractAMRPart,ClipPlane,vtkPlane);


#define CTH_AMR_SURFACE_VALUE 0.499

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

  this->IgnoreGhostLevels = 1;
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
  
  // SINCE CELL TO POINTS HANDLES BOTH ALGORITHMS, WE DO NOT
  // NEED THE GHOST LEVEL OPTION HERE.
  
  if (inputCopy->GetNumberOfGhostLevels() == 0 || this->IgnoreGhostLevels)
    {
    // Convert cell data to point data.
    vtkCTHAMRCellToPointData* cellToPoint = vtkCTHAMRCellToPointData::New();
    cellToPoint->SetInput(inputCopy);
    num = this->VolumeArrayNames->GetNumberOfStrings();
    for (idx = 0; idx < num; ++idx)
      {
      cellToPoint->AddVolumeArrayName(this->VolumeArrayNames->GetString(idx));
      }
    cellToPoint->SetIgnoreGhostLevels(this->IgnoreGhostLevels);
    cellToPoint->Update();
    inputCopy->ShallowCopy(cellToPoint->GetOutput());
    cellToPoint->Delete();    
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
      block->GetCellData()->RemoveArray(arrayName);
      }
    } 

  // Get rid of ghost cells.
  if ( ! this->IgnoreGhostLevels)
    {
    int ext[6];
    // This should really be inputCopy.
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
    }
  
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
  this->Contour->ComputeNormalsOff();
#else
  this->Contour = vtkContourFilter::New();
#endif
  this->Contour->SetInput(this->Image);
  // I had a problem with boundary.  Hotel effect.
  // The bondary surface was being clipped a hair under 0.5
  this->Contour->SetValue(0, CTH_AMR_SURFACE_VALUE);

  // Create the capping surface for the contour and append.
  this->Append1 = vtkAppendPolyData::New();
  this->Append1->AddInput(this->Contour->GetOutput());
  this->Surface = vtkDataSetSurfaceFilter::New();
  this->Surface->SetInput(this->Image);

  // Clip surface less than volume fraction 0.5.
  this->Clip0 = vtkClipPolyData::New();
  this->Clip0->SetInput(this->Surface->GetOutput());
  this->Clip0->SetValue(CTH_AMR_SURFACE_VALUE);
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
    this->Clip2->SetValue(CTH_AMR_SURFACE_VALUE);
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
void vtkCTHExtractAMRPart::ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
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
    
  os << indent << "IgnoreGhostLevels: " << this->IgnoreGhostLevels << endl;
}




