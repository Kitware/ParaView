/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSampler1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridSampler1.h"

#include "vtkObjectFactory.h"
#include "vtkIntArray.h"

vtkStandardNewMacro(vtkGridSampler1);

//define to 1 to adapt for thin k dimension
#define OCEAN_TWEAK 0

vtkGridSampler1::vtkGridSampler1()
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
vtkGridSampler1::~vtkGridSampler1()
{
  if (this->SplitPath)
    {
    this->SplitPath->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGridSampler1::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//------------------------------------------------------------------------------
void vtkGridSampler1::SetWholeExtent(int *inExtent)
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
void vtkGridSampler1::ComputeSplits(int *pathLen, int **splitPath)
{
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
    if (dims[2] >= dims[1] && dims[2] >= dims[0] && dims[2]/2 >= 1)
#endif
      {
      axis = 2;
      dims[2] = dims[2] / 2;
      }
    else if (dims[1] >= dims[0] && dims[1]/2 >= 1)
      {
      axis = 1;
      dims[1] = dims[1] / 2;
      }
    else if (dims[0]/2 >= 1)
      {
      axis = 0;
      dims[0] = dims[0] / 2;
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
double vtkGridSampler1::SuggestSampling(int axis)
{
  int dims[3];
  dims[0] = this->WholeExtent[1]-this->WholeExtent[0];
  dims[1] = this->WholeExtent[3]-this->WholeExtent[2];
  dims[2] = this->WholeExtent[5]-this->WholeExtent[4];

  double ret = 1.0;
  if (dims[axis]<0)
    {
    this->Strides[axis] = 1;
    return ret;
    }

  //minimum split divides by 30, because that looks pretty
  int iN = 30;
  //assume splitting dim in two each time
  //how many splits does it take to get till no more splits are possible?
  int root = 0;
#if OCEAN_TWEAK
  int idx = 1;
#else
  int idx = 0;
#endif
  int ldims[3];
  ldims[0] = dims[0];
  ldims[1] = dims[1];
  ldims[2] = dims[2];
  while (1)
    {
    int ax = this->SplitPath->GetValue(idx);
    if (ldims[ax]/iN <= 1)
      {
      break;
      }
    ldims[ax] = ldims[ax]/2;
    idx++;
    }
  root = idx;
  //cerr << axis << " MAX SPLITS ARE: " << root << " LD=" << ldims[0] << " " << ldims[1] << " " << ldims[2];

  //Use that as the number of nice resolution increments we want to do.
  //So aRes (the suggested resolution) is nearest multiple of 1.0/numsplits >= this->RequestedResolution
  double ir = 1.0/(double)root;
  int i;
  for (i = 0; i < root; i++)
    {
    if (ir*i > this->RequestedResolution)
      {
      break;
      }
    }
  ret = ir*i;
  //cerr << " this->RequestedResolution=" << this->RequestedResolution << " " << " i=" << i << " aRes = " << *aRes;

  //using the current res on that scale,
  //how many cells wide is each region?
  for (int j = 0; j < i-1; j++)
    {
    int ax = this->SplitPath->GetValue(j);
    dims[ax] = dims[ax] / 2;
    }
  double spaceper = dims[axis];
  //cerr << " SPACE= " << spaceper << " NATURAL = " << dims[0] << " " << dims[1] << " " << dims[2];

  //divide that by N to sample it roughly
  //also round up to 1 when get below 1 cell
  double dN = (double)iN;
  this->Strides[axis] = (int)(spaceper+dN)/iN;
  if (this->Strides[axis] == 1)
    {
    //cerr << " CLAMP STRIDE";
    ret = 1.0;
    }
  if (ret == 1.0)
    {
    //cerr << " STRIDE =" << *stride << endl;
    //cerr << " CLAMP aRes";
    this->Strides[axis] = 1;
    }
  //cerr << " STRIDE =" << *stride << endl;

  //NOTE: a table would be much faster
  return ret;
}

//------------------------------------------------------------------------------
vtkIntArray* vtkGridSampler1::GetSplitPath()
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
void vtkGridSampler1::SetSpacing(double *inSpacing)
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
void vtkGridSampler1::GetStrides(int *ret)
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
void vtkGridSampler1::GetStridedExtent(int *ret)
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
void vtkGridSampler1::GetStridedSpacing(double *ret)
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
double vtkGridSampler1::GetStridedResolution()
{
  if (!this->SamplingValid)
    {
    return -1;
    }
  return this->StridedResolution;
}

//------------------------------------------------------------------------------
void vtkGridSampler1::ComputeAtResolution(double r)
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

  double ires, jres, kres;

  //pick stride for I, J and K
  this->Strides[0] = this->Strides[1] = this->Strides[2] = 1;
  ires = this->SuggestSampling(0);
  jres = this->SuggestSampling(1);
#if OCEAN_TWEAK
#else
  kres = this->SuggestSampling(2);
#endif

  this->StridedResolution = ires;
  if (jres < this->StridedResolution)
    {
    this->StridedResolution = jres;
    }
#if OCEAN_TWEAK
#else
  if (kres < this->StridedResolution)
    {
    this->StridedResolution = kres;
    }
#endif

  //given stride result, what is low res whole extent?
  int dim[3];
  dim[0] = (this->WholeExtent[1]-this->WholeExtent[0]+1) / this->Strides[0];
  dim[1] = (this->WholeExtent[3]-this->WholeExtent[2]+1) / this->Strides[1];
  dim[2] = (this->WholeExtent[5]-this->WholeExtent[4]+1) / this->Strides[2];

  this->StridedExtent[0] = this->WholeExtent[0];
  this->StridedExtent[2] = this->WholeExtent[2];
  this->StridedExtent[4] = this->WholeExtent[4];
  this->StridedExtent[1] = this->StridedExtent[0]+dim[0]-1;
  this->StridedExtent[3] = this->StridedExtent[2]+dim[1]-1;
  this->StridedExtent[5] = this->StridedExtent[4]+dim[2]-1;

  this->StridedSpacing[0] = this->Spacing[0] * this->Strides[0];
  this->StridedSpacing[1] = this->Spacing[1] * this->Strides[1];
  this->StridedSpacing[2] = this->Spacing[2] * this->Strides[2];
}
