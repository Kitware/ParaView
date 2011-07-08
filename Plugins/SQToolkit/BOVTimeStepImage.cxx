/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "BOVTimeStepImage.h"

#include "BOVScalarImage.h"
#include "BOVVectorImage.h"

#include "BOVMetaData.h"
#include "SQMacros.h"

#include <sstream>
using std::ostringstream;

#ifdef WIN32
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
  ostringstream seriesExt;
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
      ostringstream fileName;
      fileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << seriesExt.str();

      // open
      BOVScalarImage *scalar
        = new BOVScalarImage(comm,hints,fileName.str().c_str(),arrayName);

      this->Scalars.push_back(scalar);
      }
    // vector
    else
    if (metaData->IsArrayVector(arrayName))
      {
      // deduce the file name from the following convention: 
      // arrayname{x,y,z}_step.ext
      ostringstream xFileName,yFileName,zFileName;
      xFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "x" << seriesExt.str();
      yFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "y" << seriesExt.str();
      zFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "z" << seriesExt.str();

      // open
      BOVVectorImage *vector = new BOVVectorImage;
      vector->SetName(arrayName);
      vector->SetNumberOfComponents(3);
      vector->SetComponentFile(0,comm,hints,xFileName.str().c_str());
      vector->SetComponentFile(1,comm,hints,yFileName.str().c_str());
      vector->SetComponentFile(2,comm,hints,zFileName.str().c_str());

      this->Vectors.push_back(vector);
      }
    // tensor
    else
    if (metaData->IsArrayTensor(arrayName))
      {
      // deduce the file name from the following convention: 
      // arrayname{xx,xy, ... ,zz}_step.ext
      ostringstream
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
      tensor->SetComponentFile(0,comm,hints,xxFileName.str().c_str());
      tensor->SetComponentFile(1,comm,hints,xyFileName.str().c_str());
      tensor->SetComponentFile(2,comm,hints,xzFileName.str().c_str());
      tensor->SetComponentFile(3,comm,hints,yxFileName.str().c_str());
      tensor->SetComponentFile(4,comm,hints,yyFileName.str().c_str());
      tensor->SetComponentFile(5,comm,hints,yzFileName.str().c_str());
      tensor->SetComponentFile(6,comm,hints,zxFileName.str().c_str());
      tensor->SetComponentFile(7,comm,hints,zyFileName.str().c_str());
      tensor->SetComponentFile(8,comm,hints,zzFileName.str().c_str());

      this->Tensors.push_back(tensor);
      }
    // symetric tensor
    else
    if (metaData->IsArraySymetricTensor(arrayName))
      {
      // deduce the file name from the following convention: 
      // arrayname{xx,xy, ... ,zz}_step.ext
      ostringstream
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
      symTensor->SetComponentFile(0,comm,hints,xxFileName.str().c_str());
      symTensor->SetComponentFile(1,comm,hints,xyFileName.str().c_str());
      symTensor->SetComponentFile(2,comm,hints,xzFileName.str().c_str());
      symTensor->SetComponentFile(3,comm,hints,yyFileName.str().c_str());
      symTensor->SetComponentFile(4,comm,hints,yzFileName.str().c_str());
      symTensor->SetComponentFile(5,comm,hints,zzFileName.str().c_str());

      this->SymetricTensors.push_back(symTensor);
      }
    // other ?
    else
      {
      sqErrorMacro(cerr,"Bad array type for array " << arrayName << ".");
      }
    }
}

//-----------------------------------------------------------------------------
BOVTimeStepImage::~BOVTimeStepImage()
{
  int nScalars=this->Scalars.size();
  for (int i=0; i<nScalars; ++i)
    {
    delete this->Scalars[i];
    }

  int nVectors=this->Vectors.size();
  for (int i=0; i<nVectors; ++i)
    {
    delete this->Vectors[i];
    }

  int nTensors=this->Tensors.size();
  for (int i=0; i<nTensors; ++i)
    {
    delete this->Tensors[i];
    }

  int nSymetricTensors=this->SymetricTensors.size();
  for (int i=0; i<nSymetricTensors; ++i)
    {
    delete this->SymetricTensors[i];
    }
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVTimeStepImage &si)
{
  os << "Scalars:" << endl;
  int nScalars=si.Scalars.size();
  for (int i=0; i<nScalars; ++i)
    {
    os << *si.Scalars[i] << endl;
    }

  os << "Vectors:" << endl;
  int nVectors=si.Vectors.size();
  for (int i=0; i<nVectors; ++i)
    {
    os << *si.Vectors[i] << endl;
    }

  os << "Tensors:" << endl;
  int nTensors=si.Tensors.size();
  for (int i=0; i<nTensors; ++i)
    {
    os << *si.Tensors[i] << endl;
    }

  os << "SymetricTensors:" << endl;
  int nSymetricTensors=si.SymetricTensors.size();
  for (int i=0; i<nSymetricTensors; ++i)
    {
    os << *si.SymetricTensors[i] << endl;
    }

  return os;
}
