// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMaterialInterfaceProcessLoading.h"
using std::endl;
using std::ostream;
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
