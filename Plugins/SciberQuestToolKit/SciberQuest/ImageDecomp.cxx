/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "ImageDecomp.h"

#include "CartesianDataBlock.h"
#include "CartesianDataBlockIODescriptor.h"
#include "Tuple.hxx"
#include "SQMacros.h"

// #define ImageDecompDEBUG

//-----------------------------------------------------------------------------
ImageDecomp::ImageDecomp()
{
  this->SetOrigin(0.0,0.0,0.0);
  this->SetSpacing(1.0,1.0,1.0);
}

//-----------------------------------------------------------------------------
ImageDecomp::~ImageDecomp()
{ }

//-----------------------------------------------------------------------------
void ImageDecomp::SetOrigin(double x0, double y0, double z0)
{
  double X0_l[3]={x0,y0,z0};
  this->SetOrigin(X0_l);
}

//-----------------------------------------------------------------------------
void ImageDecomp::SetOrigin(const double x0[3])
{
  this->X0[0]=x0[0];
  this->X0[1]=x0[1];
  this->X0[2]=x0[2];
}

//-----------------------------------------------------------------------------
void ImageDecomp::SetSpacing(double dx, double dy, double dz)
{
  double dX[3]={dx,dy,dz};
  this->SetSpacing(dX);
}

//-----------------------------------------------------------------------------
void ImageDecomp::SetSpacing(const double dX[3])
{
  this->DX[0]=dX[0];
  this->DX[1]=dX[1];
  this->DX[2]=dX[2];
}

//-----------------------------------------------------------------------------
void ImageDecomp::ComputeBounds()
{
  CartesianExtent::GetBounds(
        this->Extent,
        this->X0,
        this->DX,
        this->Mode,
        this->Bounds.GetData());
}

//-----------------------------------------------------------------------------
int ImageDecomp::DecomposeDomain()
{
  int nCells[3];
  this->Extent.Size(nCells);

  if ( this->DecompDims[0]>nCells[0]
    || this->DecompDims[1]>nCells[1]
    || this->DecompDims[2]>nCells[2] )
    {
    sqErrorMacro(std::cerr,
      << "Too many blocks "
      << Tuple<int>(this->DecompDims,3)
      << " requested for extent "
      << this->Extent
      << ".");
    return 0;
    }

  // free any resources allocated in a previous decomposition.
  this->ClearDecomp();
  this->ClearIODescriptors();

  size_t nBlocks
    = this->DecompDims[0]*this->DecompDims[1]*this->DecompDims[2];
  this->Decomp.resize(nBlocks,0);
  this->IODescriptors.resize(nBlocks,0);

  int smBlockSize[3]={0};
  int nLarge[3]={0};
  for (int q=0; q<3; ++q)
    {
    smBlockSize[q]=nCells[q]/this->DecompDims[q];
    nLarge[q]=nCells[q]%this->DecompDims[q];
    }

  #ifdef ImageDecompDEBUG
  std::cerr
    << "DecompDims=" << Tuple<int>(this->DecompDims,3) << std::endl
    << "nCells=" << Tuple<int>(nCells,3) << std::endl
    << "smBlockSize=" << Tuple<int>(smBlockSize,3) << std::endl
    << "nLarge=" << Tuple<int>(nLarge,3) << std::endl;
  #endif

  CartesianExtent fileExt(this->FileExtent);
  fileExt
    = CartesianExtent::CellToNode(fileExt,this->Mode); // dual grid

  int idx=0;

  // allocate and initialize each block in the new decomposition.
  // when ghosts are employed the blocks don't have ghosts so that
  // they are uniquely identified. Ghosts are handled later by  the
  // IODescriptor.
  for (int k=0; k<this->DecompDims[2]; ++k)
    {
    for (int j=0; j<this->DecompDims[1]; ++j)
      {
      for (int i=0; i<this->DecompDims[0]; ++i)
        {
        CartesianDataBlock *block=new CartesianDataBlock;

        block->SetId(i,j,k,idx);
        int *I=block->GetId();

        CartesianExtent &ext=block->GetExtent();

        for (int q=0; q<3; ++q)
          {
          int lo=2*q;
          int hi=lo+1;

          // compute extent
          if (I[q]<nLarge[q])
            {
            ext[lo]=this->Extent[lo]+I[q]*(smBlockSize[q]+1);
            ext[hi]=ext[lo]+smBlockSize[q];
            }
          else
            {
            ext[lo]=this->Extent[lo]+I[q]*smBlockSize[q]+nLarge[q];
            ext[hi]=ext[lo]+smBlockSize[q]-1;
            }
          }

        // compute bounds
        double *bounds=block->GetBounds().GetData();
        CartesianExtent::GetBounds(
              ext,
              this->X0,
              this->DX,
              this->Mode,
              bounds);

        #ifdef ImageDecompDEBUG
        std::cerr << *block << std::endl;
        #endif

        // create an io descriptor.
        CartesianExtent blockExt(ext);
        blockExt=CartesianExtent::CellToNode(blockExt,this->Mode);

        CartesianDataBlockIODescriptor *descr
            = new CartesianDataBlockIODescriptor(
                  blockExt,
                  fileExt,
                  this->PeriodicBC,
                  this->NGhosts);

        this->Decomp[idx]=block;
        this->IODescriptors[idx]=descr;
        ++idx;
        }
      }
    }

  return 1;
}
