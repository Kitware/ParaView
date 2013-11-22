/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "RectilinearDecomp.h"

#include "CartesianDataBlock.h"
#include "CartesianDataBlockIODescriptor.h"
#include "Tuple.hxx"
#include "SQMacros.h"

// #define RectilinearDecompDEBUG

//-----------------------------------------------------------------------------
RectilinearDecomp::RectilinearDecomp()
{
  this->Coordinates[0]=
  this->Coordinates[1]=
  this->Coordinates[2]=0;
}

//-----------------------------------------------------------------------------
RectilinearDecomp::~RectilinearDecomp()
{
  if (this->Coordinates[0]) this->Coordinates[0]->Delete();
  if (this->Coordinates[1]) this->Coordinates[1]->Delete();
  if (this->Coordinates[2]) this->Coordinates[2]->Delete();
}

//-----------------------------------------------------------------------------
void RectilinearDecomp::SetCoordinate(int q, SharedArray<float> *coord)
{
  if (this->Coordinates[q]==coord)
    {
    return;
    }

  if (this->Coordinates[q])
    {
    this->Coordinates[q]->Delete();
    }

  this->Coordinates[q]=coord;

  if (this->Coordinates[q])
    {
    this->Coordinates[q]->Register();
    }
}

//-----------------------------------------------------------------------------
float *RectilinearDecomp::SubsetCoordinate(int q, const CartesianExtent &ext) const
{
  int n[3];
  ext.Size(n);

  const float *coord=this->Coordinates[q]->GetPointer();
  float *scoord=(float *)malloc(n[q]*sizeof(float));

  for (int i=0,s=ext[2*q],e=ext[2*q+1]; s<=e; ++i,++s)
    {
    scoord[i]=coord[s];
    }

  return scoord;
}

//-----------------------------------------------------------------------------
int RectilinearDecomp::DecomposeDomain()
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

  #ifdef RectilinearDecompDEBUG
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
        double bounds[6];
        CartesianExtent::GetBounds(
            ext,
            this->Coordinates[0]->GetPointer(),
            this->Coordinates[1]->GetPointer(),
            this->Coordinates[2]->GetPointer(),
            this->Mode,
            bounds);
        block->GetBounds().Set(bounds);

        #ifdef RectilinearDecompDEBUG
        std::cerr << *block << std::endl;
        #endif

        // create an io descriptor.
        CartesianExtent blockExt(ext);
        blockExt
          = CartesianExtent::CellToNode(blockExt,this->Mode); //dual grid

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
