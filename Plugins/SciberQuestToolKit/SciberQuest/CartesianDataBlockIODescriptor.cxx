/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
