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
#ifndef CartesianDataBlockIODescriptor_h
#define CartesianDataBlockIODescriptor_h

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
