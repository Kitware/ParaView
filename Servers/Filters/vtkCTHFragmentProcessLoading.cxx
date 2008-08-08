/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentProcessLoading.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCTHFragmentProcessLoading.h"
using vtksys_ios::ostream;
using vtksys_ios::endl;
using vtkstd::vector;

//
ostream &operator<<(ostream &sout, vtkCTHFragmentProcessLoading &fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoadFactor() << ")";

  return sout;
}
//
ostream &operator<<(ostream &sout, vector<vtkCTHFragmentProcessLoading> &vfp)
{
  int n=vfp.size();
  vtkIdType total=0;
  for (int i=0; i<n; ++i)
    {
    total+=vfp[i].GetLoadFactor();
    sout << "(" << vfp[i].GetId() << "," << vfp[i].GetLoadFactor() << ")" << endl;
    }
  sout << "Total loading:" << total << endl;
  return sout;
}
