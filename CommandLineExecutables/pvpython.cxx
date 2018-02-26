/*=========================================================================

Program:   ParaView
Module:    pvpython.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pvpython.h" // Include this first.

#include "vtkOutputWindow.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkProcessModule.h"

#ifndef BUILD_SHARED_LIBS
#include "pvStaticPluginsInit.h"
#endif
#ifdef HAVE_NUMPY_STATIC
#include "pvnumpy-static.h"
#endif

int main(int argc, char* argv[])
{
#ifdef HAVE_NUMPY_STATIC
  paraview_numpy_static_setup();
#endif
#ifndef BUILD_SHARED_LIBS
  paraview_static_plugins_init();
#endif

  // Setup the output window to be vtkOutputWindow, rather than platform
  // specific one. This avoids creating vtkWin32OutputWindow on Windows, for
  // example, which puts all Python errors in a window rather than the terminal
  // as one would expect.
  auto opwindow = vtkOutputWindow::New();
  vtkOutputWindow::SetInstance(opwindow);
  opwindow->Delete();

  return ParaViewPython::Run(vtkProcessModule::PROCESS_CLIENT, argc, argv);
}
