/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageCacheFilter.cxx
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
#include "vtkPVImageCacheFilter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkMultiProcessController.h"
#include "vtkTableExtentTranslator.h"
#include "vtkExtentSplitter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkPVImageCacheFilter, "1.1.2.2");
vtkStandardNewMacro(vtkPVImageCacheFilter);

//----------------------------------------------------------------------------
vtkPVImageCacheFilter::vtkPVImageCacheFilter()
{
  this->Cache = vtkImageData::New();
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->Controller->Register(this);
  this->ExtentMap = vtkExtentSplitter::New();

  this->OutputAllocated = 0;
}

//----------------------------------------------------------------------------
vtkPVImageCacheFilter::~vtkPVImageCacheFilter()
{
  if (this->Cache)
    {
    this->Cache->Delete();
    this->Cache = NULL;
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
  this->ExtentMap->Delete();
  this->ExtentMap = NULL;
}


//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkIndent i2 = indent.GetNextIndent();
  
  os << indent << "Cache: " << this->Cache << endl;
  os << indent << "CacheUpdateTime: " << this->CacheUpdateTime << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkPVImageCacheFilter::UpdateData(vtkDataObject *outObject)
{
  int idx;
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();

  vtkTimerLog::MarkStartEvent("Update ImageCache");

  this->BuildExtentMap(input, output);

  // Copy all of the extents into the output.
  this->OutputAllocated = 0;
  for (idx = 0; idx < this->ExtentMap->GetNumberOfSubExtents(); ++idx)
    {
    int ext[6];
    int extProc;
    this->ExtentMap->GetSubExtent(idx, ext);
    extProc = this->ExtentMap->GetSubExtentSource(idx);
      { 
      if (extProc == 0)
        { // This extent is from the cache.
        this->CopyImageExtent(this->Cache, output, ext);
        }
      if (extProc == 1)
        { // This extent is from the input.
        input->SetUpdateExtent(ext);
        input->Update();
        this->CopyImageExtent(input, output, ext);
        }
      if (extProc > 1)
        { // This extent is from a remote processes.
        vtkErrorMacro("Remote extents are not implemented yet.");
        }
      }
    }
  
  // Save the output as cache.
  this->Cache->ShallowCopy(output);
  this->CacheUpdateTime.Modified();

  vtkTimerLog::MarkEndEvent("Update ImageCache");
}


//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::AllocateOutput(vtkImageData *out, vtkImageData* in)
{
  // Allocate the output.
  vtkIdType numCells;
  vtkIdType numPoints;
  int dimensions = 0;
  int *ext;
  ext = out->GetUpdateExtent();
  out->SetExtent(ext);
  numPoints = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
  numCells = 1;
  if (ext[1] > ext[0])
    {
    ++dimensions;
    numCells *= (ext[1]-ext[0]);
    }
  if (ext[3] > ext[2])
    {
    ++dimensions;
    numCells *= (ext[3]-ext[2]);
    }
  if (ext[5] > ext[4])
    {
    ++dimensions;
    numCells *= (ext[5]-ext[4]);
    }
  out->GetPointData()->CopyAllocate(in->GetPointData(), numPoints);
  out->GetCellData()->CopyAllocate(in->GetCellData(), numCells);

  this->OutputAllocated = 1;
}



//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::CopyImageExtent(vtkImageData *in,
                                            vtkImageData *out,
                                            int *ext)
{
  int inExt[6];
  int outExt[6];
  int cellExt[6];
  int *updateExt = out->GetUpdateExtent();
  int i, min, max;

  // Check to see if we have the the update extent in one chunk.
  if (ext[0] <= updateExt[0] && ext[1] >= updateExt[1] &&
      ext[2] <= updateExt[2] && ext[3] >= updateExt[3] &&
      ext[4] <= updateExt[4] && ext[5] >= updateExt[5])
    {
    // This fast path counts on the splitter only having one subextent
    // When it covers the whole update extent.
    out->ShallowCopy(in);
    return;
    }
  if ( ! this->OutputAllocated)
    {
    this->AllocateOutput(out, in);
    }

  in->GetExtent(inExt);
  out->GetExtent(outExt);
  this->CopyDataAttributes(ext, in->GetPointData(), inExt, 
                           out->GetPointData(), outExt);
  // We have to worry about dimensionality for cell data.
  for (i = 0; i < 3; ++i)
    {
    min = 2*i;
    max = min+1;
    cellExt[min] = ext[min];
    cellExt[max] = ext[max];
    if (inExt[min]>inExt[max] || outExt[min]>outExt[max] || ext[min]>ext[max])
      {
      vtkErrorMacro("Empty Extent");
      return;
      }
    if (inExt[min] < inExt[max])
      {
      // SanityCheck
      if ( outExt[min] == outExt[max] || ext[min] == ext[max])
        {
        vtkErrorMacro("Dimension mismatch");
        return;
        }
      --inExt[max];
      --outExt[max];
      --cellExt[max];
      }
    else
      { // This dimensions is collasped.
      // SanityCheck
      if ( outExt[min] < outExt[max] || ext[min] < ext[max] )
        {
        vtkErrorMacro("Dimension mismatch");
        return;
        }
      }
    }  

  this->CopyDataAttributes(cellExt, in->GetCellData(), inExt, 
                           out->GetCellData(), outExt);

}


//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::CopyDataAttributes(int* copyExt,
                                       vtkDataSetAttributes* in, int* inExt,
                                       vtkDataSetAttributes* out, int* outExt)
{
  /*
  int numArrays;
  int idx;
  vtkDataArray* inArray;
  vtkDataArray* outArray;


  numArrays = in->GetNumberOfArrays();
  if (numArrays != out->GetNumberOfArrays())
    {
    vtkErrorMacro("Inconsistent arrays.");
    return;
    }

  for (idx = 0; idx < numArrays; ++idx)
    {
    inArray = in->GetArray(idx);
    outArray = out->GetArray(idx);
    this->CopyArray(copyExt, inArray, inExt, outArray, outExt);
    }
  */
  
  int x, y, z;
  int xMin, xMax;
  int yMin, yMax;
  int zMin, zMax;
  vtkIdType zIdIn, yIdIn, xIdIn;
  vtkIdType zIdOut, yIdOut, xIdOut;
  vtkIdType yIncIn, zIncIn;
  vtkIdType yIncOut, zIncOut;

  xMin = copyExt[0];
  xMax = copyExt[1];
  yMin = copyExt[2];
  yMax = copyExt[3];
  zMin = copyExt[4];
  zMax = copyExt[5];
  // Increments
  yIncIn = (inExt[1]-inExt[0]+1);
  zIncIn = (inExt[3]-inExt[2]+1)*yIncIn;
  yIncOut = (outExt[1]-outExt[0]+1);
  zIncOut = (outExt[3]-outExt[2]+1)*yIncOut;
  // corner (starting id).
  zIdIn = (xMin-inExt[0]) + (yMin-inExt[2])*yIncIn + (zMin-inExt[4])*zIncIn;
  zIdOut = (xMin-outExt[0]) + (yMin-outExt[2])*yIncOut; 
  for (z = zMin; z <= zMax; ++z)
    {
    yIdIn = zIdIn;
    yIdOut = zIdOut;
    for (y = yMin; y <= yMax; ++y)
      {
      xIdIn = yIdIn;
      xIdOut = yIdOut;
      for (x = xMin; x <= xMax; ++x)
        {
        out->CopyData(in, xIdIn, xIdOut);
        xIdIn += 1;
        xIdOut += 1;
        }
      yIdIn += yIncIn;
      yIdOut += yIncOut;
      }
    zIdIn += zIncIn;
    zIdOut += zIncOut;
    }
   
}


//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkImageCacheCopyArray(int *copyExt, T *inPtr, int *inExt, 
                            T *outPtr, int outExt[6])
{
  // Note: all extents are now [min, max+1).
  int xSpan = (copyExt[1]-copyExt[0])*sizeof(T);
  T* zInPtr;
  T* yInPtr;
  T* zOutPtr;
  T* yOutPtr;
  int y, z;
  int yInInc, zInInc;
  int yOutInc, zOutInc;

  yInInc = (inExt[1]-inExt[0]);
  yOutInc = (outExt[1]-outExt[0]);
  zInInc = (inExt[3]-inExt[2])*yInInc;
  zOutInc = (outExt[3]-outExt[2])*yOutInc;

  // Move starting point to correct spot in memory.
  zInPtr = inPtr + (copyExt[0]-inExt[0]) 
                 + (copyExt[3]-inExt[3])*yInInc
                 + (copyExt[5]-inExt[5])*zInInc;
  zOutPtr = outPtr + (copyExt[0]-outExt[0]) 
                   + (copyExt[3]-outExt[3])*yOutInc
                   + (copyExt[5]-outExt[5])*zOutInc;

  for (z = copyExt[4]; z < copyExt[5]; ++z)
    {
    yInPtr = zInPtr;
    yOutPtr = zOutPtr;
    for (y = copyExt[2]; y < copyExt[3]; ++y)
      {
      memcpy(yOutPtr, yInPtr, xSpan);
      yInPtr += yInInc;
      yOutPtr += yOutInc;
      }
    zInPtr += zInInc;
    zOutPtr += zOutInc;
    }
}

//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::CopyArray(int* copyExt,
                                      vtkDataArray* in, int* inExt,
                                      vtkDataArray* out, int* outExt)
{
  int numComps;
  int copyExtC[6];
  int inExtC[6];
  int outExtC[6];

  // Although memory has been allocated, the number of tuples has not been set.
  if (out->GetNumberOfTuples() == 0)
    {
    out->SetNumberOfTuples((outExt[1]-outExt[0]+1)*
                           (outExt[3]-outExt[2]+1)*
                           (outExt[5]-outExt[4]+1));
    }

  if (in->GetDataType() != out->GetDataType())
    {
    vtkErrorMacro("Type mismatch.");
    return;
    }
  if (in->GetNumberOfComponents() != out->GetNumberOfComponents())
    {
    vtkErrorMacro("Component mismatch.");
    return;
    }

  // Get pointers
  void* inPtr = in->GetVoidPointer(0);
  void* outPtr = out->GetVoidPointer(0);

  // Convert extents to hide components.
  // Add one to the maxs.  Easier to deal with components.
  memcpy(copyExtC, copyExt, 6*sizeof(int));
  memcpy(inExtC, inExt, 6*sizeof(int));
  memcpy(outExtC, outExt, 6*sizeof(int));
  numComps = in->GetNumberOfComponents();
  copyExtC[0] = copyExtC[0]*numComps;
  copyExtC[1] = (copyExtC[1]+1)*numComps;
  copyExtC[3] = copyExtC[3]+1;
  copyExtC[5] = copyExtC[5]+1;
  inExtC[0] = inExtC[0]*numComps;
  inExtC[1] = (inExtC[1]+1)*numComps;
  inExtC[3] = inExtC[3]+1;
  inExtC[5] = inExtC[5]+1;
  outExtC[0] = outExtC[0]*numComps;
  outExtC[1] = (outExtC[1]+1)*numComps;
  outExtC[3] = outExtC[3]+1;
  outExtC[5] = outExtC[5]+1;

  switch (in->GetDataType())
    {
    vtkTemplateMacro5(vtkImageCacheCopyArray, copyExtC, 
                      (VTK_TT *)inPtr, inExtC, 
                      (VTK_TT *)outPtr, outExtC);
    
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }  
}






//----------------------------------------------------------------------------
void vtkPVImageCacheFilter::BuildExtentMap(vtkDataSet *in,
                                           vtkDataSet *out)
{
  vtkTableExtentTranslator* table;
  int myId;

  this->ExtentMap->RemoveAllExtentSources();

  // Consider cache first.
  if (this->Cache && this->CacheUpdateTime > out->GetPipelineMTime())
    {
    // Local cache has highest priority. ID = 0;
    this->ExtentMap->AddExtentSource(0, 2, this->Cache->GetExtent());
    }
  else
    {
    this->Cache->ReleaseData();
    }

  table = vtkTableExtentTranslator::SafeDownCast(out->GetExtentTranslator());
  if (1 || table == NULL)
    {
    // Add extents available locally. (id = 1).
    this->ExtentMap->AddExtentSource(1, 1, in->GetWholeExtent());
    }
  else
    {
    myId = this->Controller->GetLocalProcessId();
    // Not finished ...
    }

  // Add the extent we want to get.
  this->ExtentMap->AddExtent(out->GetUpdateExtent());
  this->ExtentMap->ComputeSubExtents();
}










