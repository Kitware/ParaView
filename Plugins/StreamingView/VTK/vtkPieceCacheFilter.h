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
// filter can be asked to agregate all cache results into a single polydata.
// Afterward a single request can obtain everything.
//
// This filter must be paired with a vtkPieceCacheExecutive. The Executive
// prevents upstream filter execution in the event of a cache hit.
//
// .SEE ALSO
// vtkPieceCacheExecutive

#ifndef __vtkPieceCacheFilter_h
#define __vtkPieceCacheFilter_h

#include "vtkDataSetAlgorithm.h"

#include <map> // used for the cache

class vtkAppendPolyData;
class vtkPolyData;

class VTK_EXPORT vtkPieceCacheFilter : public vtkDataSetAlgorithm
{
public:
  static vtkPieceCacheFilter *New();
  vtkTypeMacro(vtkPieceCacheFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the maximum number of pieces that can be retained in memory.
  // It defaults to -1, meaning unbounded.
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize,int);

  //Description:
  //Returns the dataset stored in the i'th cache slot.
  //Note: There is no SetPiece because Pieces are put into slots
  //during pipeline updates.
  vtkDataSet *GetPiece(int i);

  //Description:
  //Returns the time that dataset stored in the i'th cache slot
  //was created.
  unsigned long GetPieceMTime(int i);

  //Description:
  //Deletes the data set stored in the i'th cache slot.
  void DeletePiece(int i);

  //Description:
  //Removes all data from the cache (and append slot)
  void EmptyCache();

  //Description:
  //Removes all data from the append slot.
  void EmptyAppend();

  //Description:
  //Convert piece/number of pieces into a unique cache slot index
  int ComputeIndex(int piece, int numPieces) const
  {
    return (((piece&0x0000FFFF)<<16) | (numPieces&0x0000FFFF));
  }

  //Description:
  //Retrieve the piece number corresponding to a unique cache slot index
  int ComputePiece(int index) const
  {
    return index>>16;
  }

  //Description:
  //Retrieve the number of pieces corresponding to a unique cache slot index
  int ComputeNumberOfPieces(int index) const
  {
    return index&0x0000FFFF;
  }

  //Description:
  //Returns true if a given piece is in the cache and is stored with at
  //least the requested resolution.
  bool InCache(int piece, int numPieces, double res);

  //Description:
  //Call to append all cached vtkPolyDatas into one
  void AppendPieces();
  //Description:
  //Call to obtain the appended result.
  vtkPolyData *GetAppendedData();
  //Description:
  //Returns true if a given piece has been placed into the appended result
  //and was stored with at least the requested resolution.
  bool InAppend(int piece, int numpieces, double res);

protected:
  vtkPieceCacheFilter();
  ~vtkPieceCacheFilter();

  // Description:
  // We need to override this method because we need to use a
  // specialized executive.
  vtkExecutive* CreateDefaultExecutive();

  //Description:
  //Overriden to retrieve results from cache if present and to insert them into the
  //cache when not
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

//BTX
  //The cache is a map of slots to datasets. The datasets are stored with their
  //pipeline time so that they do not become stale.
  typedef std::map<
    int, //slot
    std::pair<
    unsigned long, //pipeline modified time
    vtkDataSet *> //data
    > CacheType;
  CacheType Cache;

  //The filter keeps track of what contents are part of the append table, along
  //with the resolution they were stored at
  typedef std::map<
    int, //slot
    double //resolution
    > AppendIndex;
  AppendIndex AppendTable;
//ETX

  int CacheSize;
  vtkAppendPolyData *AppendFilter;
  vtkPolyData *AppendResult;

private:
  vtkPieceCacheFilter(const vtkPieceCacheFilter&);  // Not implemented.
  void operator=(const vtkPieceCacheFilter&);  // Not implemented.
};


#endif
