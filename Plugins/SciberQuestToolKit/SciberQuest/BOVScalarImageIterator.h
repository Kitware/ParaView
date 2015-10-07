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
#ifndef BOVScalarImageIterator_h
#define BOVScalarImageIterator_h

#include "BOVTimeStepImage.h" // for BOVTimeStepImage
#include "BOVScalarImage.h" // for BOVScalarImage

/// Iterator for a collection of scalar handles.
class BOVScalarImageIterator
{
public:
  BOVScalarImageIterator(const BOVTimeStepImage *step)
       :
    Step(step),
    Idx(0),
    End(step->Scalars.size())
      { }

  /**
  Will evaluate true during the traversal.
  */
  virtual bool Ok() const { return this->Idx<this->End; }

  /**
  Advance the iterator.
  */
  virtual size_t Next()
    {
    if (this->Idx<this->End)
      {
      ++this->Idx;
      return this->Idx;
      }
      return 0;
    }

  /**
  Access file handle.
  */
  virtual MPI_File GetFile() const
    {
    return this->Step->Scalars[this->Idx]->GetFile();
    }

  /**
  Get array name.
  */
  virtual const char *GetName() const
    {
    return this->Step->Scalars[this->Idx]->GetName();
    }

private:
  /// \Section NotImplemented \@{
  BOVScalarImageIterator();
  BOVScalarImageIterator(const BOVScalarImageIterator &);
  void operator=(const BOVScalarImageIterator &);
  /// \@}

private:
  const BOVTimeStepImage *Step;
  size_t Idx;
  size_t End;
};

#endif

// VTK-HeaderTest-Exclude: BOVScalarImageIterator.h
