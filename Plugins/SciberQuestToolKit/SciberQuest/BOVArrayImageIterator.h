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
#ifndef BOVArrayImageIterator_h
#define BOVArrayImageIterator_h

#include "BOVScalarImage.h" // for BOVScalarImage
#include "BOVVectorImage.h" // for BOVVectorImage

/// Base class for an iterator for a collection of array handles.
/**
Base class for an iterator that traverses a collection of array
handles. For example all the scalar arrays of a given time
step.
*/
class BOVArrayImageIterator
{
public:
  BOVArrayImageIterator(const BOVTimeStepImage *step, size_t size)
       :
    Step(step),
    Idx(0),
    End(size)
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
  Get the component file handles.
  */
  virtual int GetNumberOfComponents() const = 0;
  virtual MPI_File GetComponentFile(int comp=0) const = 0;

  /**
  Get the array name.
  */
  virtual const char *GetName() const = 0;

private:
  /// \Section NotImplemented \@{
  BOVArrayImageIterator();
  BOVArrayImageIterator(const BOVArrayImageIterator &);
  void operator=(const BOVArrayImageIterator &);
  /// \@}

protected:
  const BOVTimeStepImage *Step;
  size_t Idx;
  size_t End;
};

// Define some specific classes of the iterator

#define BOVArrayImageIteratorImplMacro(name, ncomps) \
\
class BOV##name##ImageIterator : public BOVArrayImageIterator \
{ \
public: \
  BOV##name##ImageIterator(const BOVTimeStepImage *step) \
       : \
    BOVArrayImageIterator( \
          step, \
          step->name##s.size()) \
      { } \
\
  /** \
  Get number of components.\
  */ \
  virtual int GetNumberOfComponents() const \
    { \
    return ncomps; \
    } \
\
  /** \
  Get the component file handles.\
  */ \
  virtual MPI_File GetComponentFile(int comp=0) const \
    { \
    return this->Step->name##s[this->Idx]->GetComponentFile(comp); \
    } \
\
  /** \
  Get the array name.\
  */ \
  virtual const char *GetName() const \
    { \
    return this->Step->name##s[this->Idx]->GetName(); \
    } \
\
private: \
  BOV##name##ImageIterator(); \
  BOV##name##ImageIterator(const BOV##name##ImageIterator &); \
  void operator=(const BOV##name##ImageIterator &); \
}

BOVArrayImageIteratorImplMacro(Vector,3);
BOVArrayImageIteratorImplMacro(Tensor,9);
BOVArrayImageIteratorImplMacro(SymetricTensor,6);

#endif

// VTK-HeaderTest-Exclude: BOVArrayImageIterator.h
