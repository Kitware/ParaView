/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CartesianDataBlockIODescriptorIterator_h
#define __CartesianDataBlockIODescriptorIterator_h

#include "CartesianDataBlockIODescriptor.h"

/// Iterator for CartesianDataBlockIODescriptor views
class CartesianDataBlockIODescriptorIterator
{
public:
  CartesianDataBlockIODescriptorIterator(const CartesianDataBlockIODescriptor *desc)
        :
    Descriptor(desc),
    At(0),
    Of(desc->Size())
       {}

  /**
  Perpare for traversal.
  */
  void Initialize(){ this->At=0; }

  /**
  Advance to next view pair.
  */
  void Next(){ ++this->At; }

  /**
  Evaluates true during traversal.
  */
  int Ok() const { return this->At<this->Of; }

  /**
  Access to the views.
  */
  MPI_Datatype GetMemView() const { return this->Descriptor->MemViews[this->At]; }
  MPI_Datatype GetFileView() const { return this->Descriptor->FileViews[this->At]; }

private:
  /// \Section NotImplemented \@{
  CartesianDataBlockIODescriptorIterator();
  CartesianDataBlockIODescriptorIterator(const CartesianDataBlockIODescriptorIterator &);
  void operator=(const CartesianDataBlockIODescriptorIterator &);
  /// \@}

  friend std::ostream &operator<<(std::ostream &os, const CartesianDataBlockIODescriptorIterator &it);

private:
  const CartesianDataBlockIODescriptor *Descriptor;
  size_t At;
  size_t Of;
};

std::ostream &operator<<(std::ostream &os, const CartesianDataBlockIODescriptorIterator &it);

#endif

// VTK-HeaderTest-Exclude: CartesianDataBlockIODescriptorIterator.h
