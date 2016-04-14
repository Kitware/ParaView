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
#include "BOVTimeStepImage.h"

#include "BOVScalarImage.h"
#include "BOVVectorImage.h"

#include "BOVMetaData.h"
#include "SQMacros.h"

#include <sstream>

#ifdef _WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

//-----------------------------------------------------------------------------
BOVTimeStepImage::BOVTimeStepImage(
      MPI_Comm comm,
      MPI_Info hints,
      int stepIdx,
      BOVMetaData *metaData)
{
  #ifdef SQTK_WITHOUT_MPI
  sqErrorMacro(
    std::cerr,
    << "This class requires MPI but it was built without MPI.");
  (void)comm;
  (void)hints;
  (void)stepIdx;
  (void)metaData;
  #else
  int mpiMode=0;
  if (metaData->ReadMode())
    {
    mpiMode=MPI_MODE_RDONLY;
    }
  else
  if (metaData->WriteMode())
    {
    mpiMode=MPI_MODE_WRONLY|MPI_MODE_CREATE;
    }
  else
    {
    sqErrorMacro(std::cerr,"Invalid mode " << metaData->GetMode());
    }

  std::ostringstream seriesExt;
  seriesExt << "_" << stepIdx << "." << metaData->GetBrickFileExtension();
  // Open each array.
  size_t nArrays=metaData->GetNumberOfArrays();
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=metaData->GetArrayName(i);
    // skip inactive arrays
    if (!metaData->IsArrayActive(arrayName))
      {
      continue;
      }
    // scalar
    if (metaData->IsArrayScalar(arrayName))
      {
      // deduce the file name from the following convention:
      // arrayname_step.ext
      std::ostringstream fileName;
      fileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << seriesExt.str();

      // open
      BOVScalarImage *scalar
        = new BOVScalarImage(comm,hints,fileName.str().c_str(),arrayName,mpiMode);

      this->Scalars.push_back(scalar);
      }
    // vector
    else
    if (metaData->IsArrayVector(arrayName))
      {
      // deduce the file name from the following convention:
      // arrayname{x,y,z}_step.ext
      std::ostringstream xFileName,yFileName,zFileName;
      xFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "x" << seriesExt.str();
      yFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "y" << seriesExt.str();
      zFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "z" << seriesExt.str();

      // open
      BOVVectorImage *vector = new BOVVectorImage;
      vector->SetName(arrayName);
      vector->SetNumberOfComponents(3);
      vector->SetComponentFile(0,comm,hints,xFileName.str().c_str(),mpiMode);
      vector->SetComponentFile(1,comm,hints,yFileName.str().c_str(),mpiMode);
      vector->SetComponentFile(2,comm,hints,zFileName.str().c_str(),mpiMode);

      this->Vectors.push_back(vector);
      }
    // tensor
    else
    if (metaData->IsArrayTensor(arrayName))
      {
      // deduce the file name from the following convention:
      // arrayname{xx,xy, ... ,zz}_step.ext
      std::ostringstream
            xxFileName,xyFileName,xzFileName,
            yxFileName,yyFileName,yzFileName,
            zxFileName,zyFileName,zzFileName;

      xxFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xx" << seriesExt.str();
      xyFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xy" << seriesExt.str();
      xzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xz" << seriesExt.str();
      yxFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-yx" << seriesExt.str();
      yyFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-yy" << seriesExt.str();
      yzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-yz" << seriesExt.str();
      zxFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-zx" << seriesExt.str();
      zyFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-zy" << seriesExt.str();
      zzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-zz" << seriesExt.str();

      // open
      BOVVectorImage *tensor = new BOVVectorImage;
      tensor->SetName(arrayName);
      tensor->SetNumberOfComponents(9);
      tensor->SetComponentFile(0,comm,hints,xxFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(1,comm,hints,xyFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(2,comm,hints,xzFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(3,comm,hints,yxFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(4,comm,hints,yyFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(5,comm,hints,yzFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(6,comm,hints,zxFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(7,comm,hints,zyFileName.str().c_str(),mpiMode);
      tensor->SetComponentFile(8,comm,hints,zzFileName.str().c_str(),mpiMode);

      this->Tensors.push_back(tensor);
      }
    // symetric tensor
    else
    if (metaData->IsArraySymetricTensor(arrayName))
      {
      // deduce the file name from the following convention:
      // arrayname{xx,xy, ... ,zz}_step.ext
      std::ostringstream
            xxFileName,xyFileName,xzFileName,
                       yyFileName,yzFileName,
                                  zzFileName;

      xxFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xx" << seriesExt.str();
      xyFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xy" << seriesExt.str();
      xzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-xz" << seriesExt.str();
      yyFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-yy" << seriesExt.str();
      yzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-yz" << seriesExt.str();
      zzFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "-zz" << seriesExt.str();

      // open
      BOVVectorImage *symTensor = new BOVVectorImage;
      symTensor->SetName(arrayName);
      symTensor->SetNumberOfComponents(6);
      symTensor->SetComponentFile(0,comm,hints,xxFileName.str().c_str(),mpiMode);
      symTensor->SetComponentFile(1,comm,hints,xyFileName.str().c_str(),mpiMode);
      symTensor->SetComponentFile(2,comm,hints,xzFileName.str().c_str(),mpiMode);
      symTensor->SetComponentFile(3,comm,hints,yyFileName.str().c_str(),mpiMode);
      symTensor->SetComponentFile(4,comm,hints,yzFileName.str().c_str(),mpiMode);
      symTensor->SetComponentFile(5,comm,hints,zzFileName.str().c_str(),mpiMode);

      this->SymetricTensors.push_back(symTensor);
      }
    // other ?
    else
      {
      sqErrorMacro(std::cerr,"Bad array type for array " << arrayName << ".");
      }
    }
  #endif
}

//-----------------------------------------------------------------------------
BOVTimeStepImage::~BOVTimeStepImage()
{
  size_t nScalars=this->Scalars.size();
  for (size_t i=0; i<nScalars; ++i)
    {
    delete this->Scalars[i];
    }

  size_t nVectors=this->Vectors.size();
  for (size_t i=0; i<nVectors; ++i)
    {
    delete this->Vectors[i];
    }

  size_t nTensors=this->Tensors.size();
  for (size_t i=0; i<nTensors; ++i)
    {
    delete this->Tensors[i];
    }

  size_t nSymetricTensors=this->SymetricTensors.size();
  for (size_t i=0; i<nSymetricTensors; ++i)
    {
    delete this->SymetricTensors[i];
    }
}

//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const BOVTimeStepImage &si)
{
  os << "Scalars:" << std::endl;
  size_t nScalars=si.Scalars.size();
  for (size_t i=0; i<nScalars; ++i)
    {
    os << *si.Scalars[i] << std::endl;
    }

  os << "Vectors:" << std::endl;
  size_t nVectors=si.Vectors.size();
  for (size_t i=0; i<nVectors; ++i)
    {
    os << *si.Vectors[i] << std::endl;
    }

  os << "Tensors:" << std::endl;
  size_t nTensors=si.Tensors.size();
  for (size_t i=0; i<nTensors; ++i)
    {
    os << *si.Tensors[i] << std::endl;
    }

  os << "SymetricTensors:" << std::endl;
  size_t nSymetricTensors=si.SymetricTensors.size();
  for (size_t i=0; i<nSymetricTensors; ++i)
    {
    os << *si.SymetricTensors[i] << std::endl;
    }

  return os;
}
