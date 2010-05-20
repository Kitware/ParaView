/*=========================================================================

  Program:   ParaView
  Module:    vtkCPIntHolder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPIntHolder_h
#define vtkCPIntHolder_h

#include "vtkObject.h"
#include "CPWin32Header.h" // For windows import/export of shared libraries

class COPROCESSING_EXPORT vtkCPIntHolder : public vtkObject
{
public:
  static vtkCPIntHolder* New();
  vtkTypeMacro(vtkCPIntHolder,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  int GetInt();
  void SetInt(int V);

private:
  vtkCPIntHolder(const vtkCPIntHolder&); // Not implemented
  void operator=(const vtkCPIntHolder&); // Not implemented

  vtkCPIntHolder();
  ~vtkCPIntHolder();
  int Value;
};

#endif
