/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __BOVTimeStepImage_h
#define __BOVTimeStepImage_h

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
typedef void * MPI_Info;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Comm and MPI_Info
#endif

#include <vector> // for vector
#include <iostream> // for ostream

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

  size_t GetNumberOfImages() const
    {
    return
      this->Scalars.size()+
      this->Vectors.size()+
      this->Tensors.size()+
      this->SymetricTensors.size();
    }

private:
  std::vector<BOVScalarImage*> Scalars;
  std::vector<BOVVectorImage*> Vectors;
  std::vector<BOVVectorImage*> Tensors;
  std::vector<BOVVectorImage*> SymetricTensors;

private:
  BOVTimeStepImage();
  BOVTimeStepImage(const BOVTimeStepImage&);
  const BOVTimeStepImage &operator=(const BOVTimeStepImage &);

private:
  friend std::ostream &operator<<(std::ostream &os, const BOVTimeStepImage &si);
  friend class BOVScalarImageIterator;
  friend class BOVVectorImageIterator;
  friend class BOVTensorImageIterator;
  friend class BOVSymetricTensorImageIterator;
};

std::ostream &operator<<(std::ostream &os, const BOVTimeStepImage &si);

#endif

// VTK-HeaderTest-Exclude: BOVTimeStepImage.h
