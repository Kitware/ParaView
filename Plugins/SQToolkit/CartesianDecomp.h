/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __CartesianDecomp_h
#define __CartesianDecomp_h


#include "RefCountedPointer.h"
#include "CartesianExtent.h"
#include "CartesianBounds.h"

#include <vector>
using std::vector;

class CartesianDataBlock;
class CartesianDataBlockIODescriptor;


/// Splits a cartesian grid into a set of smaller cartesian grids.
/**
Splits a cartesian grid into a set of smaller cartesian grids using
a set of axis aligned planes. Given a point will locate and return 
the sub-grid which contains it.
*/
class CartesianDecomp : public RefCountedPointer
{
public:
  //static CartesianDecomp *New(){ return new CartesianDecomp; }

  /**
  Set the index space of the data on disk.
  */
  void SetFileExtent(int ilo,int ihi,int jlo,int jhi,int klo,int khi);
  void SetFileExtent(const int ext[6]);
  void SetFileExtent(const CartesianExtent &ext);
  CartesianExtent &GetFileExtent(){ return this->FileExtent; }
  const CartesianExtent &GetFileExtent() const { return this->FileExtent; }

  /**
  Get/Set the domain origin.
  */
//   void SetOrigin(double x0, double y0, double z0);
//   void SetOrigin(const double X0[3]);
//   double *GetOrigin(){ return this->X0; }
//   const double *GetOrigin() const { return this->X0; }

  /**
  Get/Set the grid spacing.
  */
//   void SetSpacing(double dx, double dy, double dz);
//   void SetSpacing(const double dX[3]);
//   double *GetSpacing(){ return this->DX; }
//   const double *GetSpacing() const { return this->DX; }

  /**
  Get/Set the physical bounds of the entire decomposition.
  */
  void SetBounds(
        double xlo,
        double xhi,
        double ylo,
        double yhi,
        double zlo,
        double zhi);
  void SetBounds(const double bounds[6]);
  void SetBounds(const CartesianBounds &bounds);
  CartesianBounds &GetBounds(){ return this->Bounds; }
  const CartesianBounds &GetBounds() const { return this->Bounds; }

  /**
  Set the domain bounds based on curren values of
  extent, oring and spacing.
  */
//   void ComputeBounds();

  /**
  Set the index space of the decomposition.
  */
  void SetExtent(int ilo,int ihi,int jlo,int jhi,int klo,int khi);
  void SetExtent(const int ext[6]);
  void SetExtent(const CartesianExtent &ext);
  CartesianExtent &GetExtent(){ return this->Extent; }
  const CartesianExtent &GetExtent() const { return this->Extent; }

  /**
  Get the dimension of the decompostion in each direction.
  */
  int SetDecompDims(int nBlocks);
  int SetDecompDims(int ni, int nj, int nk);
  int SetDecompDims(const int decompDims[3]);
  int *GetDecompDimensions(){ return this->DecompDims; }
  const int *GetDecompDimensions() const { return this->DecompDims; }

  /**
  Set the periodic boundary flags.
  */
  void SetPeriodicBC(int pi, int pj, int pk);
  void SetPeriodicBC(const int periodic[3]);
  int *GetPeriodicBC(){ return this->PeriodicBC; }
  const int *GetPeriodicBC() const { return this->PeriodicBC; }

  /**
  Set the number of ghost cells each block will have.
  */
  void SetNumberOfGhostCells(int n){ this->NGhosts=n; }
  int GetNumberOfGhostCells() const { return this->NGhosts; }

  /**
  Decompose the domain in to the requested number of blocks.
  */
//   virtual int DecomposeDomain()=0;

  /**
  Find and return the block which contains a given point.
  If no block contains the point null is returned.
  */
  CartesianDataBlock *GetBlock(const double *pt);

  /**
  Get the block at the tuple (i,j,k).
  */
  const CartesianDataBlock *GetBlock(const int *I) const
    {
    return this->GetBlock(I[0],I[1],I[2]);
    }
  CartesianDataBlock *GetBlock(const int *I)
    {
    return
      const_cast<CartesianDataBlock *>(
        static_cast<const CartesianDecomp*>(this)->GetBlock(I[0],I[1],I[2]));
    }
  const CartesianDataBlock *GetBlock(int i, int j, int k) const
    {
    const int nij=this->DecompDims[3];
    const int ni=this->DecompDims[0];
    const int idx=k*nij+j*ni+i;
    return this->Decomp[idx];
    }
  CartesianDataBlock *GetBlock(int i, int j, int k)
    {
    return
      const_cast<CartesianDataBlock *>(
        static_cast<const CartesianDecomp*>(this)->GetBlock(i,j,k));
    }

  /**
  Return the block at the given flat index.
  */
  CartesianDataBlock *GetBlock(int idx){ return this->Decomp[idx]; }
  const CartesianDataBlock *GetBlock(int idx) const { return this->Decomp[idx]; }

  /**
  Return the io descriptor at the given flat index.
  */
  CartesianDataBlockIODescriptor *GetBlockIODescriptor(int idx)
    {
    return this->IODescriptors[idx];
    }
  const CartesianDataBlockIODescriptor *GetBlockIODescriptor(int idx) const
    {
    return this->IODescriptors[idx];
    }

  /**
  Get the array of blocks.
  */
  // CartesianDataBlock **GetBlocks(){ return &this->Decomp[0]; }
  // const CartesianDataBlock * const *GetBlocks() const { return &this->Decomp[0]; }

  /**
  Get the number of blocks.
  */
  int GetNumberOfBlocks() const { return this->Decomp.size(); }

  /**
  Convert an id tuple into an index.
  */
  int BlockIdToIndex(const int *I) const
    {
    const int nij=this->DecompDims[3];
    const int ni=this->DecompDims[0];
    const int idx=I[2]*nij+I[1]*ni+I[0];
    return idx;
    }

private:
  void operator=(CartesianDecomp &); // not implemented
  CartesianDecomp(CartesianDecomp &); // not implemented 

protected:
  CartesianDecomp();
  virtual ~CartesianDecomp();

  /**
  Release decomposition resources.
  */
  virtual void ClearDecomp();
  virtual void ClearIODescriptors();

protected:
  int NGhosts;                  // number of ghost cells.
  int PeriodicBC[3];            // boolean set if periodic boundary in the direction
  int DecompDims[4];            // block array size (ni,nj,nk,ni*nj)
  vector<CartesianDataBlock *> Decomp;   // block array
  vector<CartesianDataBlockIODescriptor *> IODescriptors;
  CartesianBounds Bounds;       // domain world space bounds
  CartesianExtent Extent;       // domain index space bounds
  CartesianExtent FileExtent;   // extent of all data on disk
};

#endif

