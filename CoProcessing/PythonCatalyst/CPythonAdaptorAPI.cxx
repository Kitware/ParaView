/*=========================================================================

  Program:   ParaView
  Module:    CPythonAdaptorAPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "CPythonAdaptorAPI.h"
#include "vtkCPPythonAdaptorAPI.h"

void coprocessorinitializewithpython(
  char* pythonFileName, int* pythonFileNameLength)
{
  int length = *pythonFileNameLength;

  char *cPythonFileName = new char[length + 1];
  memcpy(cPythonFileName, pythonFileName, sizeof(char)* length);
  cPythonFileName[length] = 0;

  vtkCPPythonAdaptorAPI::CoProcessorInitialize(cPythonFileName);

  delete []cPythonFileName;
}
