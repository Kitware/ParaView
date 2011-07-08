/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVTimeStepImage_h
#define __BOVTimeStepImage_h

#include <mpi.h>

#include <vector>
using std::vector;

#include <iostream>
using std::ostream;

class BOVMetaData;

class BOVScalarImage;
class BOVVectorImage;
class BOVTensorImage;
class BOVSymetricTensorImage;

class BOVScalarImageIterator;
class BOVVectorImageIterator;
class BOVTensorImageIterator;
class BOVSymetricTensorImageIterator;

/// Collection of file handles for a single timestep.
/**
A collection of file handles to the scalar, vector, and tensor 
data that together comprise this time step.
*/
class BOVTimeStepImage
{
public:
  BOVTimeStepImage(
      MPI_Comm comm,
      MPI_Info hints,
      int stepId,
      BOVMetaData *metaData);
  ~BOVTimeStepImage();

  int GetNumberOfImages() const 
    {
    return
      this->Scalars.size()+
      this->Vectors.size()+
      this->Tensors.size()+
      this->SymetricTensors.size();
    }

private:
  vector<BOVScalarImage*> Scalars;
  vector<BOVVectorImage*> Vectors;
  vector<BOVVectorImage*> Tensors;
  vector<BOVVectorImage*> SymetricTensors;

private:
  BOVTimeStepImage();
  BOVTimeStepImage(const BOVTimeStepImage&);
  const BOVTimeStepImage &operator=(const BOVTimeStepImage &);

private:
  friend ostream &operator<<(ostream &os, const BOVTimeStepImage &si);
  friend class BOVScalarImageIterator;
  friend class BOVVectorImageIterator;
  friend class BOVTensorImageIterator;
  friend class BOVSymetricTensorImageIterator;
};

ostream &operator<<(ostream &os, const BOVTimeStepImage &si);

#endif
