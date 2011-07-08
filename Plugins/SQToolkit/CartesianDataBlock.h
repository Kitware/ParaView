/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __CartesianDataBlock_h
#define __CartesianDataBlock_h

#include <iostream>
using std::ostream;

#include "CartesianExtent.h"
#include "CartesianBounds.h"

class vtkDataSet;

/// Data and Meta-data describing data on cartesian grid.
class CartesianDataBlock
{
public:
  CartesianDataBlock();
  ~CartesianDataBlock();

  /**
  Set the block id, it's coordinates in the cartesian 
  decomposition.
  */
  void SetId(int i, int j, int k, int idx);
  void SetId(int *I);
  int *GetId(){ return this->Id; }
  /**
  Get the block's flat index.
  */
  int GetIndex(){ return this->Id[3]; }

  /**
  Set the physical area enclosed by the Block.
  */
  void SetBounds(double *bounds);
  void SetBounds(
        double xlo,
        double xhi,
        double ylo,
        double yhi,
        double zlo,
        double zhi);

  /**
  Direct acces to internal bounds data.
  */
  CartesianBounds &GetBounds(){ return this->Bounds; }

  /**
  Return non-zero if the point is contained within the block.
  */
  int Inside(double *pt){ return this->Bounds.Inside(pt); }


  /**
  Set the block's extent.
  */
  void SetExtent(int *ext);
  void SetExtent(
        int ilo,
        int ihi,
        int jlo,
        int jhi,
        int klo,
        int khi);

  /**
  Direct access to internal extent data.
  */
  CartesianExtent &GetExtent(){ return this->Extent; }

  /**
  Return the lower left corner of the block in world space.
  */
  void GetBlockOrigin(double *x0)
    {
    x0[0]=this->Bounds[0];
    x0[1]=this->Bounds[2];
    x0[2]=this->Bounds[4];
    }

  /**
  Set and get the data associated with this block.
  */
  void SetData(vtkDataSet *data);
  vtkDataSet *GetData();

  /**
  Return the decomp index of the neighboring
  block containing the given point.
  */
  // void GetNeighborId(double *pt, int *I);


private:
  CartesianDataBlock(CartesianDataBlock &other);
  void operator=(CartesianDataBlock &other);

  friend ostream &operator<<(ostream &os, CartesianDataBlock &b);

private:
  int Id[4];
  CartesianExtent Extent;
  CartesianBounds Bounds;
  vtkDataSet *Data;
};

ostream &operator<<(ostream &os, CartesianDataBlock &b);

#endif
