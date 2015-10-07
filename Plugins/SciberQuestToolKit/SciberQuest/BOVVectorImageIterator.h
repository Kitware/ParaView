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
#ifndef BOVVectorImageIterator_h
#define BOVVectorImageIterator_h

#include "BOVArrayImageIterator.h"

#include "BOVTimeStepImage.h" // for BOVTimeStepImage

/// Iterator for a collection of vector handles.
class BOVVectorImageIterator : public BOVArrayImageIterator
{
public:
  BOVVectorImageIterator(const BOVTimeStepImage *step)
       :
    BOVArrayImageIterator(
          step,
          step->Vectors.size())
      { }

  virtual int GetNumberOfComponents() const { return 3; }

  virtual MPI_File GetComponentFile(int comp) const
    {
    return this->Step->Vectors[this->Idx]->GetComponentFile(comp);
    }

  /**
  Get the array name.
  */
  virtual const char *GetName() const
    {
    return this->Step->Vectors[this->Idx]->GetName();
    }

private:
  /// \Section NotImplemented \@{
  BOVVectorImageIterator();
  BOVVectorImageIterator(const BOVVectorImageIterator &);
  void operator=(const BOVVectorImageIterator &);
  /// \@}
};

#endif

// VTK-HeaderTest-Exclude: BOVVectorImageIterator.h
