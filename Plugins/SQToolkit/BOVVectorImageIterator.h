/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVVectorImageIterator_h
#define __BOVVectorImageIterator_h

#include "BOVArrayImageIterator.h"

#include "BOVTimeStepImage.h"
#include "BOVVectorImage.h"

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
