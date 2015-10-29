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
#ifndef CartesianDataBlock_h
#define CartesianDataBlock_h

#include <iostream> // for ostream

#include "CartesianExtent.h" // for CartesianExtent
#include "CartesianBounds.h" // for CartesianBounds

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

  friend std::ostream &operator<<(std::ostream &os, CartesianDataBlock &b);

private:
  int Id[4];
  CartesianExtent Extent;
  CartesianBounds Bounds;
  vtkDataSet *Data;
};

std::ostream &operator<<(std::ostream &os, CartesianDataBlock &b);

#endif

// VTK-HeaderTest-Exclude: CartesianDataBlock.h
