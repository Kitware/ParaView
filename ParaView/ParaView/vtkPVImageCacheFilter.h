/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageCacheFilter.h
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
// .NAME vtkPVImageCacheFilter - Reuses output. Can get remote data.
// .SECTION Description
// vtkPVImageCacheFilter serves two purposes.
// First, it will resuse the output if possible.  This keeps the entire
// dataset from being loaded when the user requests another
// ghost level.
// Second, this filter examines whether requested data is available
// to this process.  If it is not, then it transmits the data
// from another process.



#ifndef __vtkPVImageCacheFilter_h
#define __vtkPVImageCacheFilter_h

#include "vtkImageToImageFilter.h"
class vtkMultiProcessController;
class vtkExtentSplitter;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkDataArray;


class VTK_EXPORT vtkPVImageCacheFilter : public vtkImageToImageFilter
{
public:
  static vtkPVImageCacheFilter *New();
  vtkTypeRevisionMacro(vtkPVImageCacheFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // We need to use update, because we may be requesting multiple
  // regions from the input (streaming).
  void UpdateData(vtkDataObject *outObject);
  
protected:
  vtkPVImageCacheFilter();
  ~vtkPVImageCacheFilter();

  void AllocateOutput(vtkImageData *out, vtkImageData *in);
  void BuildExtentMap(vtkDataSet *in, vtkDataSet *out);
  void CopyImageExtent(vtkImageData *in, vtkImageData *out, int *ext);
  void CopyDataAttributes(int* copyExt,
                          vtkDataSetAttributes* in, int* inExt,
                          vtkDataSetAttributes* out, int* outExt);
  void CopyArray(int* copyExt, vtkDataArray* in, int* inExt,
                               vtkDataArray* out, int* outExt);
                                      
  vtkImageData *Cache;
  vtkTimeStamp CacheUpdateTime;
  int OutputAllocated;

  vtkMultiProcessController *Controller;

  vtkExtentSplitter *ExtentMap;

private:
  vtkPVImageCacheFilter(const vtkPVImageCacheFilter&);  // Not implemented.
  void operator=(const vtkPVImageCacheFilter&);  // Not implemented.
};



#endif



