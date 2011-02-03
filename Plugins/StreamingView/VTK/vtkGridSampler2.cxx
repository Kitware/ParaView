
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSampler2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridSampler2.h"

#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkAdaptiveOptions.h"

#include <math.h>

#ifdef WIN32
  double log2(double value)
  {
    return log(value)/log(2.0);
  }
#endif

vtkStandardNewMacro(vtkGridSampler2);

//define to 1 to adapt for thin k dimension
#define OCEAN_TWEAK 0

vtkGridSampler2::vtkGridSampler2()
{
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
  this->SplitPath = NULL;
  this->PathValid = true;

  this->Strides[0] = this->Strides[1] = this->Strides[2] = 1;
  this->StridedExtent[0] = this->StridedExtent[2] = this->StridedExtent[4] = 0;
  this->StridedExtent[1] = this->StridedExtent[3] = this->StridedExtent[5] = -1;
  this->StridedResolution = 0.0;

  this->StridedSpacing[0] = 1.0;
  this->StridedSpacing[1] = 1.0;
  this->StridedSpacing[2] = 1.0;

  this->SamplingValid = false;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1;
  this->RequestedResolution = -1.0;
}

//----------------------------------------------------------------------------
vtkGridSampler2::~vtkGridSampler2()
{
  if (this->SplitPath)
    {
    this->SplitPath->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGridSampler2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//------------------------------------------------------------------------------
void vtkGridSampler2::SetWholeExtent(int *inExtent)
{
  for (int i = 0; i < 6; i++)
    {
    if (this->WholeExtent[i] != inExtent[i])
      {
      this->PathValid = false;
      this->SamplingValid = false;
      this->WholeExtent[i] = inExtent[i];
      }
    }
  return;
}

//------------------------------------------------------------------------------
void vtkGridSampler2::ComputeSplits(int *pathLen, int **splitPath)
{
  // fixed constant too to reflect sampling rate per level
  int rate = vtkAdaptiveOptions::GetRate();

  int dims[3];
  dims[0] = this->WholeExtent[1]-this->WholeExtent[0];
  dims[1] = this->WholeExtent[3]-this->WholeExtent[2];
  dims[2] = this->WholeExtent[5]-this->WholeExtent[4];

  //compute a split path (choice of axis to split at each turn)
  int buflen = 100;
  int *buffer = new int[buflen]; //allocate a buffer for result split path
  int *splits = buffer;
  *pathLen = 0;
  int axis = 0;

  while (axis > -1)
    {
    axis = -1;
#if OCEAN_TWEAK
    if (*pathLen == 0)
#else
    if (dims[2] >= dims[1] && dims[2] >= dims[0] && dims[2] / rate >= 1)
#endif
      {
      axis = 2;
      dims[2] = dims[2] / rate + (dims[2] % rate > 0 ? 1 : 0);
      }
    else if (dims[1] >= dims[0] && dims[1] / rate >= 1)
      {
      axis = 1;
      dims[1] = dims[1] / rate + (dims[1] % rate > 0 ? 1 : 0);
      }
    else if (dims[0] / rate >= 1)
      {
      axis = 0;
      dims[0] = dims[0] / rate + (dims[0] % rate > 0 ? 1 : 0);
      }

    splits[*pathLen] = axis;
    *pathLen = *pathLen + 1;

    if (*pathLen==buflen)
      {
      //protect against overflow of the allocated result buffer
      int *newbuf = new int[buflen*2];
      memcpy(newbuf, splits, buflen*sizeof(int));
      buflen = buflen*2;
      delete[] splits;
      splits = newbuf;
      }
    }

  *splitPath = splits;
}

//----------------------------------------------------------------------------
double vtkGridSampler2::SuggestSampling(int axis)
{
  // fixed constants at the moment because max level (height) is not passed
  // in the pipeline as far as I can tell
  // also need to specify the branching factor (degree) as well
  int height = vtkAdaptiveOptions::GetHeight();
  int degree =
    (int)log2((double)vtkAdaptiveOptions::GetDegree());  // 2^degree
  int rate = vtkAdaptiveOptions::GetRate();  // should be able to specify the sampling rate

  if(this->RequestedResolution >= 1.0 || height <= 0) {
    return 1;
  }

  double sampling = 1.0;

  // the last index to check for splits
  vtkIdType stop =
    (vtkIdType)(height * degree * (1.0 - this->RequestedResolution) + 0.5);

  stop = stop > this->SplitPath->GetNumberOfTuples() ?
    this->SplitPath->GetNumberOfTuples() : stop;

  // this isn't exactly correct, since the split path is just a linear
  // specification of splits, it could split twice in the same
  // dimension, instead of spliting across two dimensions...
  // this is probably OK
  for(vtkIdType i = 0; i < stop; i = i + 1) {
    // count the axes
    if(this->SplitPath->GetValue(i) == axis) {
      sampling = sampling * rate;
    }
  }

  return sampling;
}

//------------------------------------------------------------------------------
vtkIntArray* vtkGridSampler2::GetSplitPath()
{
  if (this->PathValid)
    {
    return this->SplitPath;
    }
  this->PathValid = true;
  if (this->SplitPath != NULL)
    {
    this->SplitPath->Delete();
    }
  int pathLen;
  int *path;
  this->ComputeSplits(&pathLen, &path);
  this->SplitPath = vtkIntArray::New();
  this->SplitPath->SetNumberOfComponents(1);
  this->SplitPath->SetArray(path, vtkIdType(pathLen), 0);
  return this->SplitPath;
}

//------------------------------------------------------------------------------
void vtkGridSampler2::SetSpacing(double *inSpacing)
{
  for (int i = 0; i < 3; i++)
    {
    if (this->Spacing[i] != inSpacing[i])
      {
      this->SamplingValid = false;
      this->Spacing[i] = inSpacing[i];
      }
    }
  return;
}

//------------------------------------------------------------------------------
void vtkGridSampler2::GetStrides(int *ret)
{
  if (!this->SamplingValid)
    {
    return;
    }
  ret[0] = this->Strides[0];
  ret[1] = this->Strides[1];
  ret[2] = this->Strides[2];
}

//------------------------------------------------------------------------------
void vtkGridSampler2::GetStridedExtent(int *ret)
{
  if (!this->SamplingValid)
    {
    return;
    }
  ret[0] = this->StridedExtent[0];
  ret[1] = this->StridedExtent[1];
  ret[2] = this->StridedExtent[2];
  ret[3] = this->StridedExtent[3];
  ret[4] = this->StridedExtent[4];
  ret[5] = this->StridedExtent[5];
}

//------------------------------------------------------------------------------
void vtkGridSampler2::GetStridedSpacing(double *ret)
{
  if (!this->SamplingValid)
    {
    return;
    }
  ret[0] = this->StridedSpacing[0];
  ret[1] = this->StridedSpacing[1];
  ret[2] = this->StridedSpacing[2];
}

//------------------------------------------------------------------------------
double vtkGridSampler2::GetStridedResolution()
{
  if (!this->SamplingValid)
    {
    return -1;
    }
  return this->StridedResolution;
}

//------------------------------------------------------------------------------
void vtkGridSampler2::ComputeAtResolution(double r)
{
  if (r < 0.0)
    {
    r = 0.0;
    }
  if (r > 1.0)
    {
    r = 1.0;
    }
  if (this->RequestedResolution == r && this->SamplingValid)
    {
    return;
    }

  this->SamplingValid = true;
  this->RequestedResolution = r;

  this->Strides[0] = (int)this->SuggestSampling(0);
  this->Strides[1] = (int)this->SuggestSampling(1);
  this->Strides[2] = (int)this->SuggestSampling(2);

  /*
  cout << this->Strides[0] << " "
       << this->Strides[1] << " "
       << this->Strides[2] << "\n";
  */

  //given stride result, what is low res whole extent?
  int dim[3];
  dim[0] = (this->WholeExtent[1] - this->WholeExtent[0] + 1)
    / this->Strides[0] +
    ((this->WholeExtent[1] - this->WholeExtent[0] + 1) %
     this->Strides[0] > 0 ? 1 : 0);
  dim[1] = (this->WholeExtent[3] - this->WholeExtent[2] + 1)
    / this->Strides[1] +
    ((this->WholeExtent[3] - this->WholeExtent[2] + 1) %
     this->Strides[1] > 0 ? 1 : 0);
  dim[2] = (this->WholeExtent[5] - this->WholeExtent[4] + 1)
    / this->Strides[2] +
    ((this->WholeExtent[5] - this->WholeExtent[4] + 1) %
     this->Strides[2] > 0 ? 1 : 0);

  this->StridedExtent[0] = this->WholeExtent[0];
  this->StridedExtent[2] = this->WholeExtent[2];
  this->StridedExtent[4] = this->WholeExtent[4];
  this->StridedExtent[1] = this->StridedExtent[0] + dim[0] - 1;
  this->StridedExtent[3] = this->StridedExtent[2] + dim[1] - 1;
  this->StridedExtent[5] = this->StridedExtent[4] + dim[2] - 1;

  this->StridedSpacing[0] = this->Spacing[0] * this->Strides[0];
  this->StridedSpacing[1] = this->Spacing[1] * this->Strides[1];
  this->StridedSpacing[2] = this->Spacing[2] * this->Strides[2];
}
