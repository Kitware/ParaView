// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pvpython.h" // Include this first.

#include "vtkOutputWindow.h"
#include "vtkProcessModule.h"

#if defined(_WIN32) && !defined(__MINGW32__)
int wmain(int argc, wchar_t* wargv[])
#else
int main(int argc, char* argv[])
#endif
{
#if defined(_WIN32) && !defined(__MINGW32__)
  vtkWideArgsConverter converter(argc, wargv);
  char** argv = converter.GetArgs();
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
