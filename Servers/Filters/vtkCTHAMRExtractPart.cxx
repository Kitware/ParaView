/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRExtractPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRExtractPart.h"
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
#include "vtkPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCTHAMRCellToPointData.h"
#ifdef VTK_USE_PATENTED
#  include "vtkKitwareContourFilter.h"
#  include "vtkSynchronizedTemplates3D.h"
#  include "vtkKitwareCutter.h"
#else
#  include "vtkContourFilter.h"
#  include "vtkCutter.h"
#endif
#include "vtkStringList.h"
#include "vtkPlane.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkCTHAMRSurface.h"

vtkCxxRevisionMacro(vtkCTHAMRExtractPart, "1.1");
vtkStandardNewMacro(vtkCTHAMRExtractPart);
vtkCxxSetObjectMacro(vtkCTHAMRExtractPart,ClipPlane,vtkPlane);


#define CTH_AMR_SURFACE_VALUE 0.499

//----------------------------------------------------------------------------
vtkCTHAMRExtractPart::vtkCTHAMRExtractPart()
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
  this->Clip1 = 0;
  this->Clip2 =0;
  this->Cut = 0;

  this->IgnoreGhostLevels = 1;
}

//----------------------------------------------------------------------------
vtkCTHAMRExtractPart::~vtkCTHAMRExtractPart()
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
unsigned long vtkCTHAMRExtractPart::GetMTime()
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
void vtkCTHAMRExtractPart::RemoveAllVolumeArrayNames()
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
void vtkCTHAMRExtractPart::AddVolumeArrayName(char* arrayName)
{
  vtkPolyData* d = vtkPolyData::New();
  int num = this->GetNumberOfOutputs();
  this->VolumeArrayNames->AddString(arrayName);
  this->SetOutput(num, d);
  d->Delete();
  d = NULL;
}

//--------------------------------------------------------------------------
int vtkCTHAMRExtractPart::GetNumberOfVolumeArrayNames()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}

//--------------------------------------------------------------------------
const char* vtkCTHAMRExtractPart::GetVolumeArrayName(int idx)
{
  return this->VolumeArrayNames->GetString(idx);
}

//--------------------------------------------------------------------------
void vtkCTHAMRExtractPart::SetOutput(int idx, vtkPolyData* d)
{
  this->Superclass::SetNthOutput(idx, d);  
}

//----------------------------------------------------------------------------
vtkPolyData* vtkCTHAMRExtractPart::GetOutput(int idx)
{
  return (vtkPolyData *) this->Superclass::GetOutput(idx); 
}

//----------------------------------------------------------------------------
int vtkCTHAMRExtractPart::GetNumberOfOutputs()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}


//----------------------------------------------------------------------------
void vtkCTHAMRExtractPart::Execute()
{
  int blockId, numBlocks;
  vtkCTHData* input = this->GetInput();
  vtkCTHData* inputCopy = vtkCTHData::New();
  vtkPolyData* output;
  vtkImageData* block; 
  vtkAppendPolyData** tmps;
  int idx, num;

  vtkGarbageCollector::DeferredCollectionPush();

  this->CreateInternalPipeline();
  inputCopy->ShallowCopy(input);

  vtkTimerLog::MarkStartEvent("CellToPoint");
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
  vtkTimerLog::MarkEndEvent("CellToPoint");

  // Create an append for each part (one part per output).
  num = this->VolumeArrayNames->GetNumberOfStrings();
  tmps = new vtkAppendPolyData* [num];
  for (idx = 0; idx < num; ++idx)
    {
    tmps[idx] = vtkAppendPolyData::New();
    }

  this->UpdateProgress(.05);

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = inputCopy->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    double startProg = .05 + .9 * static_cast<double>(blockId)/static_cast<double>(numBlocks); 
    double endProg = .05 + .9 * static_cast<double>(blockId+1)/static_cast<double>(numBlocks); 
    this->UpdateProgress(startProg);
    block = vtkImageData::New();
    inputCopy->GetBlock(blockId, block);
    this->ExecuteBlock(block, tmps, startProg, endProg);
    block->Initialize();
    block->Delete();
    block = NULL;
    }
  
  // Add the capped surface to the parts.
  vtkCTHAMRSurface* surface = vtkCTHAMRSurface::New();
  surface->SetInput(inputCopy);
  vtkClipPolyData* clip = vtkClipPolyData::New();
  clip->SetInput(surface->GetOutput());
  clip->SetValue(CTH_AMR_SURFACE_VALUE);
  if (this->ClipPlane)
    {
    // We need another clip for the plan.  Sneak it in.
    vtkClipPolyData *clip2 = clip;
    clip = vtkClipPolyData::New();
    clip->SetInput(clip2->GetOutput());
    clip->SetClipFunction(this->ClipPlane);
    clip2->Delete();
    }
  for (idx = 0; idx < num; ++idx)
    {
    const char* arrayName = this->VolumeArrayNames->GetString(idx);
    inputCopy->GetPointData()->SetActiveScalars(arrayName);
    clip->Update();
    tmps[idx]->AddInput(clip->GetOutput());
    // Copy appends to output (one part per output).
    this->UpdateProgress(.95 + .05 * static_cast<double>(idx)/static_cast<double>(num));
    output = this->GetOutput(idx);
 
    vtkTimerLog::MarkStartEvent("BlockAppend");               
    tmps[idx]->Update();
    vtkTimerLog::MarkEndEvent("BlockAppend");              
    
    output->ShallowCopy(tmps[idx]->GetOutput());
    // Hopping to avoid some garbage collection time.
    tmps[idx]->RemoveAllInputs();
    tmps[idx]->Delete();
    tmps[idx] = 0;

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
    arrayName = this->VolumeArrayNames->GetString(idx);
    char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
    sprintf(str, "%s", arrayName);
    output->GetFieldData()->AddArray(nameArray);
    nameArray->Delete();
    }
  surface->SetInput(0);
  surface->Delete();
  surface = 0;
  clip->SetInput(0);
  clip->Delete();
  clip = 0;
  delete [] tmps;
  tmps = NULL;
  inputCopy->Delete();
  inputCopy = NULL;
  this->DeleteInternalPipeline();
  vtkGarbageCollector::DeferredCollectionPop();  
}

//-----------------------------------------------------------------------------
void vtkCTHAMRExtractPart::ExecuteBlock(vtkImageData* block, 
                                        vtkAppendPolyData** tmps,
                                        double startProg, double endProg)
{
  vtkFloatArray* pointVolumeFraction;
  int idx, num;
  const char* arrayName;

  // Loop over parts to convert volume fractions to point arrays.
  num = this->VolumeArrayNames->GetNumberOfStrings();
  for (idx = 0; idx < num; ++idx)
    {
    this->UpdateProgress(startProg + .3 * (endProg - startProg) * static_cast<double>(idx)/static_cast<double>(num));
    arrayName = this->VolumeArrayNames->GetString(idx);
    pointVolumeFraction = (vtkFloatArray*)(block->GetPointData()->GetArray(arrayName));
    if (pointVolumeFraction == NULL)
      {
      vtkErrorMacro("Could not find point array.");
      }
    } 
  
  // Loop over parts extracting surfaces.
  for (idx = 0; idx < num; ++idx)
    {
    this->UpdateProgress(startProg + .7 * (endProg - startProg) * static_cast<double>(idx)/static_cast<double>(num));
    arrayName = this->VolumeArrayNames->GetString(idx);
    this->ExecutePart(arrayName, block, tmps[idx]);
    } 
}


//------------------------------------------------------------------------------
void vtkCTHAMRExtractPart::CreateInternalPipeline()
{
  // Having inputs keeps us from having to set and remove inputs.
  // The garbage collecting associated with this is expensive.
  this->Image = vtkImageData::New();
  
  // Note: I had trouble with the kitware contour filter setting the input
  // of the internal synchronized templates filter (garbage collection took too long.)
  
  // Create the contour surface.
#ifdef VTK_USE_PATENTED
  vtkSynchronizedTemplates3D* tmp = vtkSynchronizedTemplates3D::New();
  // vtkDataSetSurfaceFilter does not generate normals, so they will be lost.
  tmp->ComputeNormalsOff();
  tmp->SetInput(this->Image);
  // I had a problem with boundary.  Hotel effect.
  // The bondary surface was being clipped a hair under 0.5
  tmp->SetValue(0, CTH_AMR_SURFACE_VALUE);
  this->Contour = tmp;
#else
  vtkContourFilter* tmp = vtkContourFilter::New();
  tmp->SetInput(this->Image);
  // I had a problem with boundary.  Hotel effect.
  // The boundary surface was being clipped a hair under 0.5
  tmp->SetValue(0, CTH_AMR_SURFACE_VALUE);
  this->Contour = tmp;  
#endif

  // Create the capping surface for the contour and append.
  this->Append1 = vtkAppendPolyData::New();
  this->Append1->AddInput(this->Contour->GetOutput());
  
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
    this->PolyData = this->Append2->GetOutput();
    }
  else
    {
    this->PolyData = this->Append1->GetOutput();
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRExtractPart::DeleteInternalPipeline()
{
  if (this->Image)
    {
    this->Image->Delete();
    this->Image = 0;
    }

  if (this->Clip1)
    {
    //this->Clip1->SetInput(0);
    this->Clip1->Delete();
    this->Clip1 = 0;
    }
  if (this->Cut)
    {
    //this->Cut->SetInput(0);
    this->Cut->Delete();
    this->Cut = 0;
    }
  if (this->Clip2)
    {
    //this->Clip2->SetInput(0);    
    this->Clip2->Delete();
    this->Clip2 = 0;
    }
  if (this->Contour)
    {
    //this->Contour->SetInput(0);
    this->Contour->Delete();
    this->Contour = 0;
    }
  if (this->Append1)
    {
    // Make sure all temporary filters actually delete.
    //this->RemoveAllInputs();
    this->Append1->SetOutput(NULL);
    this->Append1->Delete();
    this->Append1 = 0;
    }
  if (this->Append2)
    {
    // Make sure all temperary fitlers actually delete.
    //this->Append2->RemoveAllInputs();    
    this->Append2->SetOutput(NULL);
    this->Append2->Delete();
    this->Append2 = 0;
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRExtractPart::ExecutePart(const char* arrayName,
                                       vtkImageData* block, 
                                       vtkAppendPolyData* append)
{
  // See if we can skip this block.
  vtkDataArray* array = block->GetPointData()->GetArray(arrayName);
  if (array == 0)
    {
    return;
    }
  double *range = array->GetRange();
  if (range[0] > CTH_AMR_SURFACE_VALUE || range[1] < CTH_AMR_SURFACE_VALUE)
    {
    return;
    }
  
  block->GetPointData()->SetActiveScalars(arrayName);

  this->Image->ShallowCopy(block);
  this->PolyData->Update();
  vtkPolyData* tmp = vtkPolyData::New();
  tmp->ShallowCopy(this->PolyData);
  append->AddInput(tmp);
  tmp->Delete();
}


//------------------------------------------------------------------------------
void vtkCTHAMRExtractPart::PrintSelf(ostream& os, vtkIndent indent)
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




