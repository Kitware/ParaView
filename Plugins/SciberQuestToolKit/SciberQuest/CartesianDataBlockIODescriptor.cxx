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

#include "CartesianDataBlockIODescriptor.h"

#include "Tuple.hxx"
#include "MPIRawArrayIO.hxx"

// #define CartesianDataBlockIODescriptorDEBUG

//-----------------------------------------------------------------------------
CartesianDataBlockIODescriptor::CartesianDataBlockIODescriptor(
      const CartesianExtent &blockExt,
      const CartesianExtent &fileExt,
      const int periodic[3],
      int nGhosts)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)blockExt;
  (void)fileExt;
  (void)periodic;
  (void)nGhosts;
  sqErrorMacro(
    std::cerr,
    << "This class requires MPI but it was built without MPI.");
  #else
  this->Mode
    = CartesianExtent::GetDimensionMode(fileExt,nGhosts);

  // Determine the true memory extents. Start by assuming that the
  // block is on the interior of the domain decomposition. Strip edge
  // and face ghost cells, if the block lies on the exterior of the
  // decomposition and periodic boundary conditions have not been
  // called out in that direction.
  CartesianExtent &memExt=this->MemExtent;
  memExt.Set(blockExt);
  if (nGhosts>0)
    {
    memExt=CartesianExtent::Grow(memExt,nGhosts,this->Mode);

    for (int q=0; q<3; ++q)
      {
      int qq=2*q;

      // on low side non-periodic boundary in q direction
      if (!periodic[q] && (blockExt[qq]==fileExt[qq]))
        {
        // strip the ghost cells
        memExt=CartesianExtent::GrowLow(memExt,q,-nGhosts,this->Mode);
        }

      // on high side non-periodic boundary in q direction
      if (!periodic[q] && (blockExt[qq+1]==fileExt[qq+1]))
        {
        // strip the ghost cells
        memExt=CartesianExtent::GrowHigh(memExt,q,-nGhosts,this->Mode);
        }
      }
    }

  MPI_Datatype view;
  int nFileExt[3];
  fileExt.Size(nFileExt);

  // shift and intersect the true memory extent with the file extent
  // in all permutations to compute regions to be used in the IO calls.
  // These will be empty for blocks that are interior of the domain
  // decomposition.
  for (int k=-1; k<2; ++k)
    {
    for (int j=-1; j<2; ++j)
      {
      for (int i=-1; i<2; ++i)
        {
        CartesianExtent fileRegion(memExt);
        fileRegion.Shift(i*nFileExt[0],j*nFileExt[1],k*nFileExt[2]);
        fileRegion&=fileExt;

        if (!fileRegion.Empty())
          {
          CreateCartesianView<float>(fileExt,fileRegion,view);
          this->FileViews.push_back(view);

          CartesianExtent memRegion(fileRegion);
          memRegion.Shift(-i*nFileExt[0],-j*nFileExt[1],-k*nFileExt[2]);

          CreateCartesianView<float>(memExt,memRegion,view);
          this->MemViews.push_back(view);

          #ifdef CartesianDataBlockIODescriptorDEBUG
          int regSize[3];
          memRegion.Size(regSize);
          std::cerr
            << "    "
            << fileRegion << " -> " << memRegion
            << " n=" << Tuple<int>(regSize,3) << std::endl;
          #endif
          }
        }
      }
    }
  #endif
}

//-----------------------------------------------------------------------------
CartesianDataBlockIODescriptor::~CartesianDataBlockIODescriptor()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void CartesianDataBlockIODescriptor::Clear()
{
  #ifndef SQTK_WITHOUT_MPI
  size_t n;
  n=this->MemViews.size();
  for (size_t i=0; i<n; ++i)
    {
    MPI_Type_free(&this->MemViews[i]);
    }
  this->MemViews.clear();

  n=this->FileViews.size();
  for (size_t i=0; i<n; ++i)
    {
    MPI_Type_free(&this->FileViews[i]);
    }
  this->FileViews.clear();
  #endif
}

//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os,const CartesianDataBlockIODescriptor &descr)
{
  size_t n=descr.MemViews.size();
  for (size_t i=0; i<n; ++i)
    {
    os << "    " << descr.FileViews[i] << " -> " << descr.MemViews[i] << std::endl;
    }
  return os;
}
