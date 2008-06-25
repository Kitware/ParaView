/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentUtils.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkCTHFragmentUtil_h
#define __vtkCTHFragmentUtil_h

// Vtk containers
#include<vtkDoubleArray.h>
#include<vtkFloatArray.h>
#include<vtkIntArray.h>
#include<vtkUnsignedIntArray.h>
#include<vtkDataArraySelection.h>
// STL
#include<vtkstd/vector>
using vtkstd::vector;
#include<vtkstd/string>
using vtkstd::string;
// other
#include<assert.h>

// some useful functionality that stradles multiple filters
// and has file scope.
namespace {
// vector memory management helper
template<class T>
void ClearVectorOfVtkPointers( vector<T *> &V )
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    if (V[i]!=0)
      {
      V[i]->Delete();
      }
    }
  V.clear();
}
// vector memory management helper
template<class T>
void ClearVectorOfArrayPointers( vector<T *> &V )
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    if (V[i]!=0)
      {
      delete [] V[i];
      }
    }
  V.clear();
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkPointers(vector<T *>&V, int n)
{
  ClearVectorOfVtkPointers(V);

  V.resize(n);
  for (int i=0; i<n; ++i)
    {
    V[i]=T::New();
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfArrayPointers(vector<T *>&V, int nV, int nA)
{
  ClearVectorOfArrayPointers(V);

  V.resize(nV);
  for (int i=0; i<nV; ++i)
    {
    V[i] = new T[nA];
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        vtkIdType nTups,
        string name,
        int nv)
{
  ClearVectorOfVtkPointers(V);

  V.resize(nv);
  for (int i=0; i<nv; ++i)
    {
    V[i]=T::New();
    V[i]->SetNumberOfComponents(nComps);
    V[i]->SetNumberOfTuples(nTups);
    V[i]->SetName(name.c_str());
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        int nv)
{
  ResizeVectorOfVtkArrayPointers(V,nComps,0,"",nv);
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        int nTups,
        int nv)
{
  ResizeVectorOfVtkArrayPointers(V,nComps,nTups,"",nv);
}

// vtk object memory managment helper
template<class T>
inline
void ReNewVtkPointer(T *&pv)
{
  if (pv!=0)
    {
    pv->Delete();
    }
  pv=T::New();
}
// vtk object memory managment helper
template<class T>
inline
void NewVtkArrayPointer(
                T *&pv,
                int nComps,
                vtkIdType nTups,
                vtkstd::string name)
{
  pv=T::New();
  pv->SetNumberOfComponents(nComps);
  pv->SetNumberOfTuples(nTups);
  pv->SetName(name.c_str());
}
// vtk object memory managment helper
template<class T>
inline
void ReNewVtkArrayPointer(
                T *&pv,
                int nComps,
                vtkIdType nTups,
                vtkstd::string name)
{
  if (pv!=0)
    {
    pv->Delete();
    }
  NewVtkArrayPointer(pv,nComps,nTups,name);
}
// vtk object memory managment helper
template<class T>
inline
void ReNewVtkArrayPointer(T *&pv,vtkstd::string name)
{
  ReNewVtkArrayPointer(pv,1,0,name);
}
// vtk object memory managment helper
template<class T>
inline
void ReleaseVtkPointer(T *&pv)
{
  assert("Attempted to release a 0 pointer."
         && pv!=0 );
  pv->Delete();
  pv=0;
}
// vtk object memory managment helper
template<class T>
inline
void CheckAndReleaseVtkPointer(T *&pv)
{
  if (pv==0)
    {
    return;
    }
  pv->Delete();
  pv=0;
}
// memory managment helper
template<class T>
inline
void CheckAndReleaseArrayPointer(T *&pv)
{
  if (pv==0)
    {
    return;
    }
  delete [] pv;
  pv=0;
}
// zero vector
template<class T>
inline
void FillVector( vector<T> &V, const T &v)
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    V[i]=v;
    }
}
// Copier to copy from an array where type is not known 
// into a destination buffer.
// returns 0 if the type of the source array
// is not supported.
template<class T>
inline
int CopyTuple(T *dest,          // scalar/vector
              vtkDataArray *src,//
              int nComps,       //
              int srcCellIndex) // weight of contribution
{
  // convert cell index to array index
  int srcIndex=nComps*srcCellIndex;
  // copy
  switch ( src->GetDataType() )
    {
    case VTK_FLOAT:{
      float *thisTuple
        = dynamic_cast<vtkFloatArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_DOUBLE:{
      double *thisTuple
        = dynamic_cast<vtkDoubleArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_INT:{
      int *thisTuple
        = dynamic_cast<vtkIntArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_UNSIGNED_INT:{
      unsigned int *thisTuple
        = dynamic_cast<vtkUnsignedIntArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    default:
    assert( "This data type is unsupported."
            && 0 );
    return 0;
    break;
    }
  return 1;
}
// vtk data array selection helper
int GetEnabledArrayNames(vtkDataArraySelection *das, vector<string> &names)
{
  int nEnabled=das->GetNumberOfArraysEnabled();
  names.resize(nEnabled);
  int nArraysTotal=das->GetNumberOfArrays();
  for (int i=0,j=0; i<nArraysTotal; ++i)
    {
    // skip disabled arrays
    if ( !das->GetArraySetting(i) )
      {
      continue;
      }
    // save names of enabled arrays, inc name count
    names[j]=das->GetArrayName(i);
    ++j;
    }
  return nEnabled;
}
};

#endif
