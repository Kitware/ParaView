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
#ifndef CartesianDataBlockIODescriptorIterator_h
#define CartesianDataBlockIODescriptorIterator_h

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
