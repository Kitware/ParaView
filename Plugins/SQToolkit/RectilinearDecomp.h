/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __RectilinearDecomp_h
#define __RectilinearDecomp_h


#include "CartesianDecomp.h"
#include "CartesianExtent.h"
#include "CartesianBounds.h"
#include "SharedArray.hxx"

class CartesianDataBlock;
class CartesianDataBlockIODescriptor;


/// Splits a cartesian grid into a set of smaller cartesian grids.
/**
Splits a cartesian grid into a set of smaller cartesian grids using
a set of axis aligned planes. Given a point will locate and return 
the sub-grid which contains it.
*/
class RectilinearDecomp : public CartesianDecomp
{
public:
  static RectilinearDecomp *New(){ return new RectilinearDecomp; }

  /**
  Set the qth coordinate array.
  */
  void SetCoordinate(int q, SharedArray<float> *coord);

  /**
  Decompose the domain in to the requested number of blocks.
  */
  int DecomposeDomain();

  /**
  Return he subset of he qth coordinate coresponding to the given extent.
  */
  float *SubsetCoordinate(int q, const CartesianExtent &ext) const;

protected:
  RectilinearDecomp();
  ~RectilinearDecomp();

private:
  void operator=(RectilinearDecomp &); // not implemented
  RectilinearDecomp(RectilinearDecomp &); // not implemented 

private:
  SharedArray<float> *Coordinates[3];
};

#endif

