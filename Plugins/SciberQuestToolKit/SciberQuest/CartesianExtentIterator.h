/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CartesianExtentIterator_h
#define __CartesianExtentIterator_h

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
