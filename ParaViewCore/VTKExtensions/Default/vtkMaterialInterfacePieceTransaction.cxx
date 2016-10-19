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

#include "vtkMaterialInterfacePieceTransaction.h"
using std::ostream;

//
ostream& operator<<(ostream& sout, const vtkMaterialInterfacePieceTransaction& ta)
{
  sout << "(" << ta.GetType() << "," << ta.GetRemoteProc() << ")";

  return sout;
}
