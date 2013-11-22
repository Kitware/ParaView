/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __BOVArrayImageIterator_h
#define __BOVArrayImageIterator_h

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
