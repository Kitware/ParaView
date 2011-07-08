/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __CartesianDataBlockIODescriptor_h
#define __CartesianDataBlockIODescriptor_h

#include "CartesianExtent.h"

#include <mpi.h>
#include <vector>
using std::vector;
#include <iostream>
using std::ostream;

/// Container for the MPI file and memory views
/**
Container for the MPI file and memory views required to 
read in a cartesian block of data with ghost cells. Here
the ghost cells are filled directly from disk as
no other blocks are assumed to be in memory. The views 
are accessed via CartesianDataBlockIODescriptorIterator.
*/
class CartesianDataBlockIODescriptor
{
public:
  CartesianDataBlockIODescriptor(
      const CartesianExtent &blockExt,
      const CartesianExtent &fileExt,
      const int Periodic[3],
      int nGhosts);
  ~CartesianDataBlockIODescriptor();

  /**
  Release allocated views.
  */
  void Clear();

  /**
  Get the number of views.
  */
  int Size() const { return this->MemViews.size(); }

  /**
  Access to a specific view.
  */
  MPI_Datatype GetMemView(int i) const { return this->MemViews[i]; }
  MPI_Datatype GetFileView(int i) const { return this->FileViews[i]; }

  /**
  Get the extent of the array to hold the data.
  */
  const CartesianExtent &GetMemExtent() const { return this->MemExtent; }

private:
  /// \Section NotImplemented \@{
  CartesianDataBlockIODescriptor();
  CartesianDataBlockIODescriptor(const CartesianDataBlockIODescriptor &);
  CartesianDataBlockIODescriptor &operator=(const CartesianDataBlockIODescriptor &other);
  /// \@}

  friend class CartesianDataBlockIODescriptorIterator;
  friend ostream &operator<<(ostream &os,const CartesianDataBlockIODescriptor &descr);

private:
  CartesianExtent MemExtent;
  vector<MPI_Datatype> FileViews;
  vector<MPI_Datatype> MemViews;
};

ostream &operator<<(ostream &os,const CartesianDataBlockIODescriptor &descr);

#endif
