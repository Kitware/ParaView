// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pvserver_common.h"
#if PARAVIEW_USE_PYTHON && PARAVIEW_USE_EXTERNAL_VTK
#include "vtkPVPythonInterpreterPath.h"
#include "vtkPythonInterpreter.h"
#endif

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#if PARAVIEW_USE_PYTHON && PARAVIEW_USE_EXTERNAL_VTK
  vtkPVPythonInterpreterPath();
#endif
  // Init current process type
  return RealMain(argc, argv, vtkProcessModule::PROCESS_DATA_SERVER);
}
