/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredCacheFilter.h
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
// .NAME vtkStructuredCacheFilter - Reuses output. Can get remote data.
// .SECTION Description
// vtkStructuredCacheFilter serves two purposes.
// First, it will resuse the output if possible.  This keeps the entire
// dataset from being loaded when the user requests another
// ghost level.
// Second, this filter examines whether requested data is available
// to this process.  If it is not, then it transmits the data
// from another process.



#ifndef __vtkStructuredCacheFilter_h
#define __vtkStructuredCacheFilter_h

#include "vtkDataSetToDataSetFilter.h"
class vtkMultiProcessController;
class vtkExtentSplitter;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkDataArray;


class VTK_EXPORT vtkStructuredCacheFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkStructuredCacheFilter *New();
  vtkTypeRevisionMacro(vtkStructuredCacheFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // We need to use update, because we may be requesting multiple
  // regions from the input (streaming).
  void UpdateData(vtkDataObject *outObject);
  
protected:
  vtkStructuredCacheFilter();
  ~vtkStructuredCacheFilter();

  void BuildExtentMap(vtkDataSet *in, vtkDataSet *out);
  void CopyExtent(vtkDataSet *in, vtkDataSet *out, int *ext);
  void CopyDataAttributes(int* copyExt,
                          vtkDataSetAttributes* in, int* inExt,
                          vtkDataSetAttributes* out, int* outExt);
  void CopyArray(int* copyExt, vtkDataArray* in, int* inExt,
                               vtkDataArray* out, int* outExt);

  void AllocateOutput(vtkDataSet *out, vtkDataSet *in);
  void GetExtent(vtkDataSet* ds, int ext[6]);

  vtkDataSet *Cache;
  vtkTimeStamp CacheUpdateTime;
  int OutputAllocated;

  vtkMultiProcessController *Controller;

  vtkExtentSplitter *ExtentMap;

private:
  vtkStructuredCacheFilter(const vtkStructuredCacheFilter&);  // Not implemented.
  void operator=(const vtkStructuredCacheFilter&);  // Not implemented.
};



#endif



