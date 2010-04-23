/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceCacheFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPieceCacheFilter - cache pieces for fast reuse
// .SECTION Description
// vtkPieceCacheFilter caches the pieces given to it in its input and then
// passes the data through to its output. Later requests will then
// reuse the cached piece data when present. 
//
// Additionally, if the data type stored in the cache is vtkPolyData, the cache
// filter will agregate all cache results into a single polydata, storing the
// aggregated result into the first filled cache slot. This helps speed up 
// rendering in the streaming paraview application because all cached data can
// be rerendered in a single pipeline update pass.
//
// This filter must be paired with a vtkPieceCacheExecutive, so that cache hits
// prevent upstream filters from executing, otherwise no time is saved.
//
// .SEE ALSO
// vtkPieceCacheExecutive

#ifndef __vtkPieceCacheFilter_h
#define __vtkPieceCacheFilter_h

#include "vtkDataSetAlgorithm.h"

#include <vtkstd/map> // used for the cache

class vtkAppendPolyData;

class VTK_EXPORT vtkPieceCacheFilter : public vtkDataSetAlgorithm
{
public:
  static vtkPieceCacheFilter *New();
  vtkTypeMacro(vtkPieceCacheFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the maximum number of time steps that can be retained in memory.
  // it defaults to -1, meaning unbounded.
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize,int);

  // Description:
  // Removes all data from the cache.
  void EmptyCache();

  // Description:
  // Returns the data set stored in the i'th cache slot.
  vtkDataSet *GetPiece(int i);

  // Description:
  // Deletes the data set stored in the i'th cache slot. Resetting Append slot
  // if necessary.
  void DeletePiece(int i);

  //Description:
  //Convert piece and number of pieces into a unique cache slot index
  int ComputeIndex(int piece, int numPieces) const
  {
    return (((piece&0x0000FFFF)<<16) | (numPieces&0x0000FFFF));
  }

  //Description:
  //Retrieve the piece number from a unique cache slot index
  int ComputePiece(int index) const
  {
    return index>>16;
  }

  //Description:
  //Retrieve the number of pieces from a unique cache slot index
  int ComputeNumberOfPieces(int index) const
  {
    return index&0x0000FFFF;
  }

protected:
  vtkPieceCacheFilter();
  ~vtkPieceCacheFilter();

  virtual int ProcessRequest(vtkInformation *,
                             vtkInformationVector **,
                             vtkInformationVector *);
  
  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);


//BTX
  typedef vtkstd::map<
    int, //slot
    vtkstd::pair<
    unsigned long, //pipeline modified time
    vtkDataSet *> //data
    > CacheType;
  CacheType Cache;
//ETX

  int CacheSize;
  int EnableStreamMessages;

  int TryAppend;
  vtkAppendPolyData *AppendFilter;
  int AppendSlot;

private:
  vtkPieceCacheFilter(const vtkPieceCacheFilter&);  // Not implemented.
  void operator=(const vtkPieceCacheFilter&);  // Not implemented.
};


#endif
