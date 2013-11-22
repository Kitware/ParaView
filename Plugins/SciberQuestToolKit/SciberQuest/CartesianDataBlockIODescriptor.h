/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CartesianDataBlockIODescriptor_h
#define __CartesianDataBlockIODescriptor_h

#include "CartesianExtent.h"

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Datatype;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Datatype
#endif

#include <vector> // for vector

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
  size_t Size() const { return this->MemViews.size(); }

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
  friend std::ostream &operator<<(std::ostream &os,const CartesianDataBlockIODescriptor &descr);

private:
  int Mode;
  CartesianExtent MemExtent;
  std::vector<MPI_Datatype> FileViews;
  std::vector<MPI_Datatype> MemViews;
};

std::ostream &operator<<(std::ostream &os,const CartesianDataBlockIODescriptor &descr);

#endif

// VTK-HeaderTest-Exclude: CartesianDataBlockIODescriptor.h
