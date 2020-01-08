/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMaterialInterfaceProcessLoading.h"
using std::ostream;
using std::endl;
using std::vector;

//
ostream& operator<<(ostream& sout, const vtkMaterialInterfaceProcessLoading& fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoadFactor() << ")";

  return sout;
}
//
ostream& operator<<(ostream& sout, const vector<vtkMaterialInterfaceProcessLoading>& vfp)
{
  size_t n = vfp.size();
  vtkIdType total = 0;
  for (size_t i = 0; i < n; ++i)
  {
    total += vfp[i].GetLoadFactor();
    sout << "(" << vfp[i].GetId() << "," << vfp[i].GetLoadFactor() << ")" << endl;
  }
  sout << "Total loading:" << total << endl;
  return sout;
}
