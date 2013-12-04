/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef CUDAThreadedIterator_h
#define CUDAThreadedIterator_h

#include <cuda.h> // standard cuda header
#include <cuda_runtime.h> //

/// CUDAThreadedIterator -- Iterator that works with CUDA
/**
CUDAThreadedIterator -- Iterator that works with CUDA. A flat array is
broken into blocks then each blck is broken into chunks. A cuda thread
processes each chunk. The iterator initializes itself based on the
runtime environment on the GPU to visit all of the indices owned by
the thread that created it.

NOTE:
this partitioning leads to adjacent threads accessing
data that is chunkSize away which is bad.
*/
class CUDAThreadedIterator
{
public:
  ///
  __device__ CUDAThreadedIterator() : Start(0), End(0), At(0) {}

  ///
  __device__ CUDAThreadedIterator(unsigned long dataSize)
  {
    // work is coarsely partitioned into some number of blocks
    unsigned long nLargeBlocks=dataSize%gridDim.x;
    unsigned long blockSize=dataSize/gridDim.x;

    unsigned long blockId=blockIdx.x;
    unsigned long localBlockSize;
    unsigned long localBlockStart;
    if (blockId<nLargeBlocks)
      {
      localBlockSize=blockSize+1;
      localBlockStart=blockId*localBlockSize;
      }
    else
      {
      localBlockSize=blockSize;
      localBlockStart=blockId*localBlockSize+nLargeBlocks;
      }

    // blocks are further partioned into some number of threads
    // each thread operates on a chunk of a block.
    unsigned long threadId=threadIdx.x;
    unsigned long localChunkSize;
    unsigned long nLargeChunks=localBlockSize%blockDim.x;
    unsigned long chunkSize=localBlockSize/blockDim.x;
    if (threadId<nLargeChunks)
      {
      localChunkSize=chunkSize+1;
      this->Start=localBlockStart+threadId*localChunkSize;
      }
    else
      {
      localChunkSize=chunkSize;
      this->Start=localBlockStart+threadId*localChunkSize+nLargeChunks;
      }

    // [Start End) is the index space domain of this thread.
    this->At=this->Start;
    this->End=this->Start+localChunkSize;
  }

  ///
  __device__ void Initialize(){ this->At=this->Start; }

  ///
  __device__ void Next(){ ++this->At; }

  ///
  __device__ int Ok(){ return this->At<this->End; }

  ///
  __device__ unsigned long operator()(){ return this->At; }

private:
  unsigned long Start;
  unsigned long End;
  unsigned long At;
};

#endif

// VTK-HeaderTest-Exclude: CUDAThreadedIterator.h
