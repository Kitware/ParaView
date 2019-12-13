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
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "pvpythonmodules.h"
#include "vtkUtilitiesPythonInitializerModule.h"
#include "vtkpythonmodules.h"

extern "C" {

void VTKUTILITIESPYTHONINITIALIZER_EXPORT vtkPVInitializePythonModules()
{
  vtkpythonmodules_load();
  pvpythonmodules_load();
}
}
