/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef CUDA3DDecomp_h
#define CUDA3DDecomp_h

#include <cuda.h> // standard cuda header
#include <cuda_runtime.h> //

/**
A flat array is
broken into blocks of number of threads where each adjacent thread
accesses adjacent memory locations. To accomplish this we might need
a large number of blocks. If the number of blocks exceeds the max
block dimension in the first and or second block grid dimension
then we need to use a 2d or 3d block grid.

PartitionBlocks - decides on a partitioning of the data based
on warpsPerBlock parameter. The resulting decomposition will
be either 1,2, or 2d as needed to accomodate the number of
fixed sized blocks. It can happen that max grid dimensions are
hit, in which case you'll need to increase the number of warps
per block.

ThreadIdToArrayIndex - given a thread and block id gets the
array index to update. This may be out of bounds so be sure
to validate before using it.

IndexIsValid - test an index for validity.
*/

///
__device__
unsigned long ThreadIdToArrayIndex()
{
  return
    threadIdx.x + blockDim.x*(blockIdx.x + blockIdx.y*gridDim.x + blockIdx.z*gridDim.x*gridDim.y);
}

///
__device__
int IndexIsValid(unsigned long index, unsigned long maxIndex)
{
  return index<maxIndex;
}

///
__host__
int PartitionBlocks(
      unsigned long dataSize,
      unsigned long warpsPerBlock,
      unsigned long warpSize,
      unsigned int *blockGridMax,
      dim3 &blockGrid,
      unsigned long &nBlocks,
      dim3 &threadGrid)
{
  unsigned long threadsPerBlock=warpsPerBlock*warpSize;
  threadGrid.x=threadsPerBlock;
  threadGrid.y=1;
  threadGrid.z=1;
  unsigned long blockSize=threadsPerBlock;
  nBlocks=dataSize/blockSize;
  if (dataSize%blockSize)
    {
    ++nBlocks;
    }

  if (nBlocks>blockGridMax[0])
    {
    // multi-d decomp required
    blockGrid.x=blockGridMax[0];
    blockGrid.y=nBlocks/blockGridMax[0];
    if (nBlocks%blockGridMax[0])
      {
      ++blockGrid.y;
      }
    if (blockGrid.y>blockGridMax[1])
      {
      // 3d decomp
      unsigned long blockGridMax01=blockGridMax[0]*blockGridMax[1];
      blockGrid.y=blockGridMax[1];
      blockGrid.z=nBlocks/blockGridMax01;
      if (nBlocks%blockGridMax01)
        {
        ++blockGrid.z;
        }
      if (blockGrid.z>blockGridMax[2])
        {
        sqErrorMacro(
          std::cerr,
            << "Too many blocks " << nBlocks
            << " of size " << blockSize
            << " required for a grid of ("
            << blockGridMax[0] << ", " << blockGridMax[1] << ", " << blockGridMax[2]
            << ") blocks. Hint: increase the number of warps per block.");
        return -1;
        }
      else
        {
        return 0;
        }
      }
    else
      {
      // 2d decomp
      blockGrid.z=1;
      }
    }
  else
    {
    // 1d decomp
    blockGrid.x=nBlocks;
    blockGrid.y=1;
    blockGrid.z=1;
    }
  return 0;
}

#endif

// VTK-HeaderTest-Exclude: CUDA3DDecomp.h
