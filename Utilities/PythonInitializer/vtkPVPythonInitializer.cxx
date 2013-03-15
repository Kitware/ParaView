/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"
#include "pvpythonmodules.h"
#include "vtkUtilitiesPythonInitializerModule.h"
#ifdef __cplusplus
extern "C" {
#endif
  void VTKUTILITIESPYTHONINITIALIZER_EXPORT vtkPVInitializePythonModules()
    {
    CMakeLoadAllPythonModules();
    }
#ifdef __cplusplus
}
#endif
