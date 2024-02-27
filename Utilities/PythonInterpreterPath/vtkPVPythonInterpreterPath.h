// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkUtilitiesPythonInterpreterPathModule.h"

extern "C"
{
  void VTKUTILITIESPYTHONINTERPRETERPATH_EXPORT vtkPVPythonInterpreterPath();
}
