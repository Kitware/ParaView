// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkPVPythonInterpreterPath.h"
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkResourceFileLocator.h"
#include <string>

extern "C"
{
  void vtkPVInitializePythonModules();
  void VTKUTILITIESPYTHONINTERPRETERPATH_EXPORT vtkPVPythonInterpreterPath()
  {
    std::string libraryPath = vtkGetLibraryPathForSymbol(vtkPVInitializePythonModules);
    vtkPythonInterpreter::AddUserPythonPath(
      libraryPath.c_str(), "paraview/__init__.py" /*landmark*/);
  }
}
