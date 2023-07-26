// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "vtkMaterialInterfacePieceTransaction.h"
using std::ostream;

//
ostream& operator<<(ostream& sout, const vtkMaterialInterfacePieceTransaction& ta)
{
  sout << "(" << ta.GetType() << "," << ta.GetRemoteProc() << ")";

  return sout;
}
