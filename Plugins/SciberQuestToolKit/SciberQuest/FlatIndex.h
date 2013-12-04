/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __FlatIndex_h
#define __FlatIndex_h
#include <cstdlib> // for size_t

class CartesianExtent;

/// FlatIndex - A class to convert i,j,k tuples into flat indices
/**
The following formula is applied:
<pre>
mode -> k*(A)    + j*(B)    +i*(C)
--------------------------------------
3d   -> k*(ninj) + j*(ni)   + i
xy   ->            j*(ni)   + i
xz   -> k*(ni)              + i
yz   -> k*(nj)   + j
--------------------------------------
</pre>
*/
class FlatIndex
{
public:
  FlatIndex() : A(0), B(0), C(0) {}
  FlatIndex(int ni, int nj, int nk, int mode);
  FlatIndex(const CartesianExtent &ext, int nghost=0);

  void Initialize(int ni, int nj, int nk, int mode);
  void Initialize(const CartesianExtent &ext, int nghost=0);

  size_t Index(int i, int j, int k)
  {
    return k*this->A + j*this->B + i*this->C;
  }

private:
  size_t A;
  size_t B;
  size_t C;
};

#endif

// VTK-HeaderTest-Exclude: FlatIndex.h
