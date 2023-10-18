// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "pvincubatorpythonmodules.h"
#include "pvpythonmodules.h"
#include "vtkUtilitiesPythonInitializerModule.h"
#include "vtkpythonmodules.h"

extern "C"
{

  void VTKUTILITIESPYTHONINITIALIZER_EXPORT vtkPVInitializePythonModules()
  {
    vtkpythonmodules_load();
    pvpythonmodules_load();
    pvincubatorpythonmodules_load();
  }
}
