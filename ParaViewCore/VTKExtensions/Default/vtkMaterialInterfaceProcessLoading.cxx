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
using vtksys_ios::ostream;
using vtksys_ios::endl;
using std::vector;

//
ostream &operator<<(ostream &sout, vtkMaterialInterfaceProcessLoading &fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoadFactor() << ")";

  return sout;
}
//
ostream &operator<<(ostream &sout, vector<vtkMaterialInterfaceProcessLoading> &vfp)
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
