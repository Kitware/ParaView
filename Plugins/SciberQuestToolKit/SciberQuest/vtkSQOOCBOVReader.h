/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef vtkSQOOCBOVReader_h
#define vtkSQOOCBOVReader_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkSQOOCReader.h"
#include "RefCountedPointer.h" // for SetRefCountedPointer

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Comm
#endif

#include <vector> // for vector

class vtkDataSet;
class vtkImageData;
class BOVReader;
class BOVTimeStepImage;
class CartesianDecomp;
class CartesianDataBlock;
template<typename T> class PriorityQueue;

/// Implementation for Brick-Of-Values (BOV) Out-Of-Core (OOC) file access.
/**
Allow one to read in chunks of data as needed. A specific
chunk of data is identified to be read by providing a point
in which the chunk should reside.
*/
class VTKSCIBERQUEST_EXPORT vtkSQOOCBOVReader : public vtkSQOOCReader
{
public:
  static vtkSQOOCBOVReader *New();
  vtkTypeMacro(vtkSQOOCBOVReader, vtkSQOOCReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
  Set the communicator to open the file on. optional.
  If not explicitly set the default is MPI_COMM_SELF.
  */
  virtual void SetCommunicator(MPI_Comm comm);

  /**
  Set the reader object.
  */
  SetRefCountedPointer(Reader,BOVReader);


  /// \section Cache \@{
  /**
  Set the domain decomposition to use during reads.
  */
  SetRefCountedPointer(DomainDecomp,CartesianDecomp);

  /**
  Set the number of block to cache during out-of-core operation.
  Setting the cache size greater than the number of blocks in the
  decomposition results in in-core operation, with multiple reads.
  */
  vtkSetMacro(BlockCacheSize,int);
  vtkGetMacro(BlockCacheSize,int);

  /**
  After the set'ing of a domain and cahche size the cache must
  be initialized prior to any attempt to read data.
  */
  void InitializeBlockCache();

  /**
  Empty any cached data.
  */
  void ClearBlockCache();

  /**
  If set block cache is explicitly emptied during close.
  The default is set. If you have the memory to spare you may want
  unset this.
  */
  vtkSetMacro(CloseClearsCachedBlocks,int);
  vtkGetMacro(CloseClearsCachedBlocks,int);

  /// \@}


  /// \section IO \@{
  /**
  Open the dataset for reading. In the case of an error 0 is
  returned.
  */
  virtual int Open();

  /**
  Close the dataset.
  */
  virtual void Close();

  /**
  Return the dataset containing point, p, and its valid bounds,
  in WorkingDomain. These may differ from the bounds of the dataset
  for example in the case where ghost cells are provided.
  */
  virtual vtkDataSet *ReadNeighborhood(
      const double p[3],
      CartesianBounds &WorkingDomain);

  /**
  Turn on an array to be read.
  */
  virtual void ActivateArray(const char *arrayName);
  /**
  Turn off an array to be read.
  */
  virtual void DeActivateArray(const char *arrayName);
  virtual void DeActivateAllArrays();
  /// \@}

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQOOCBOVReader();
  virtual ~vtkSQOOCBOVReader();

private:
  vtkSQOOCBOVReader(const vtkSQOOCBOVReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQOOCBOVReader&) VTK_DELETE_FUNCTION;

private:
  BOVReader *Reader;                            // reader
  BOVTimeStepImage *Image;                      // file handle
  unsigned long int BlockAccessTime;            // lru access time
  int BlockCacheSize;                           // number of block to keep in memory
  CartesianDecomp *DomainDecomp;                // domain decomposition
  PriorityQueue<unsigned long int> *LRUQueue;   // least-recently-used block queue
  int CloseClearsCachedBlocks;                  // controls cache flush on close

  std::vector<int> CacheHit;                    // count the number of times each block is accessed
  std::vector<int> CacheMiss;                   // count the number of times each block is accessed
  long long CacheHitCount;                      // track block cache hits
  long long CacheMissCount;                     // track block cache misses

  int LogLevel;                                 // enable logging
};

#endif
