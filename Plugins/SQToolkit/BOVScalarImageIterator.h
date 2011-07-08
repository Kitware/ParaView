/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVScalarImageIterator_h
#define __BOVScalarImageIterator_h

#include "BOVTimeStepImage.h"
#include "BOVScalarImage.h"

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
  virtual int Next()
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
  int Idx;
  int End;
};

#endif
