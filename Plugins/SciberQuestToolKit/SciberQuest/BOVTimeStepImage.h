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
#ifndef BOVTimeStepImage_h
#define BOVTimeStepImage_h

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
