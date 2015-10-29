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
#ifndef CartesianExtentIterator_h
#define CartesianExtentIterator_h

#include "CellIdIterator.h"
#include "CartesianExtent.h" // for CartesianExtent
#include "FlatIndex.h" // for FlatIndex
#include <cstdlib> // for size_t

/// extent iterator
/**
Iterate over an exetent in FORTRAN array order.
*/
class CartesianExtentIterator : public CellIdIterator
{
public:
  // constructors
  CartesianExtentIterator();
  CartesianExtentIterator(const CartesianExtent &extent);
  CartesianExtentIterator(const CartesianExtent &domain,const CartesianExtent &extent);
  CartesianExtentIterator(const CartesianExtentIterator &other);

  virtual ~CartesianExtentIterator(){}

  // copy
  virtual CellIdIterator &operator=(const CellIdIterator &other);
  CartesianExtentIterator &operator=(const CartesianExtentIterator &other);

  // initialize
  void SetDomain(CartesianExtent &domain);
  void SetExtent(CartesianExtent &extent);

  // begin
  virtual void Reset();
  // validate
  virtual int Good(){ return this->R<=this->Extent[5]; }
  // access
  virtual size_t Index(){ return this->Indexer.Index(this->P,this->Q,this->R); }
  virtual size_t operator*(){ return this->Index(); }
  // increment
  virtual CellIdIterator &Increment();
  virtual CellIdIterator &operator++(){ return this->Increment(); }

  // size of the extent
  virtual size_t Size(){ return this->Extent.Size(); }

private:
  FlatIndex Indexer;
  CartesianExtent Extent;
  int P;
  int Q;
  int R;
};

//-----------------------------------------------------------------------------
inline
CellIdIterator &CartesianExtentIterator::Increment()
{
  this->P+=1;
  if (this->P>this->Extent[1])
    {
    this->P=this->Extent[0];
    this->Q+=1;
    if (this->Q>this->Extent[3])
      {
      this->Q=this->Extent[2];
      this->R+=1;
      //  if we're out of bounds in k dir
      // then we are done. so don't reset R.
      }
    }
  return *this;
}

#endif

// VTK-HeaderTest-Exclude: CartesianExtentIterator.h
