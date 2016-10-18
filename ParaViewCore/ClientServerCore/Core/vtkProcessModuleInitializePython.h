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
#ifndef vtkProcessModulePythonInitializePython_h
#define vtkProcessModulePythonInitializePython_h

#include "vtkPSystemTools.h"
#include "vtkPVConfig.h" //  needed for PARAVIEW_FREEZE_PYTHON
#include "vtkPythonInterpreter.h"
#include <string>
#include <vtksys/SystemTools.hxx>

/* The maximum length of a file name.  */
#if defined(PATH_MAX)
#define VTK_PYTHON_MAXPATH PATH_MAX
#elif defined(MAXPATHLEN)
#define VTK_PYTHON_MAXPATH MAXPATHLEN
#else
#define VTK_PYTHON_MAXPATH 16384
#endif
namespace
{
// ParaView should setup Python Path to find the modules in the following
// locations for various platforms. There are always two cases to handle: when
// running from build-location and when running from installed-location.

//----------------------------------------------------------------------------
void vtkPythonAppInitPrependPythonPath(const std::string& dir)
{
  if (dir != "")
  {
    std::string collapsed_dir = vtkPSystemTools::CollapseFullPath(dir.c_str());
    if (vtkPSystemTools::FileIsDirectory(collapsed_dir.c_str()))
    {
      vtkPythonInterpreter::PrependPythonPath(collapsed_dir.c_str());
    }
  }
}

#ifndef PARAVIEW_FREEZE_PYTHON
#if defined(_WIN32)
void vtkPythonAppInitPrependPathWindows(const std::string& SELF_DIR);
#elif defined(__APPLE__)
void vtkPythonAppInitPrependPathOsX(const std::string& SELF_DIR);
#else
void vtkPythonAppInitPrependPathLinux(const std::string& SELF_DIR);
#endif
#endif // ifndef PARAVIEW_FREEZE_PYTHON

//----------------------------------------------------------------------------
void vtkPythonAppInitPrependPath(const std::string& SELF_DIR)
{
// We don't initialize Python paths when Frozen Python is being used. This
// avoid unnecessary file-system accesses..
#ifndef PARAVIEW_FREEZE_PYTHON
#if defined(_WIN32)
  vtkPythonAppInitPrependPathWindows(SELF_DIR);
#elif defined(__APPLE__)
  vtkPythonAppInitPrependPathOsX(SELF_DIR);
#else
  vtkPythonAppInitPrependPathLinux(SELF_DIR);
#endif
#endif // ifndef PARAVIEW_FREEZE_PYTHON

// *** The following maybe obsolete. Need to verify and remove. ***

// This executable does not actually link to the python wrapper
// libraries, though it probably should now that the stub-modules
// are separated from them.  Since it does not we have to make
// sure the wrapper libraries can be found by the dynamic loader
// when the stub-modules are loaded.  On UNIX this executable must
// be running in an environment where the main VTK libraries (to
// which this executable does link) have been found, so the
// wrapper libraries will also be found.  On Windows this
// executable may have simply found its .dll files next to itself
// so the wrapper libraries may not be found when the wrapper
// modules are loaded.  Solve this problem by adding this
// executable's location to the system PATH variable.  Note that
// this need only be done for an installed VTK because in the
// build tree the wrapper modules are in the same directory as the
// wrapper libraries.
#if defined(_WIN32)
  static char system_path[(VTK_PYTHON_MAXPATH + 1) * 10] = "PATH=";
  strcat(system_path, SELF_DIR.c_str());
  if (char* oldpath = getenv("PATH"))
  {
    strcat(system_path, ";");
    strcat(system_path, oldpath);
  }
  putenv(system_path);
#endif // if defined(_WIN32)
}

#ifndef PARAVIEW_FREEZE_PYTHON
#if defined(_WIN32)
//===========================================================================
// Windows
// Key:
//    - SELF_DIR: directory containing the pvserver/pvpython/paraview
//      executables.
//---------------------------------------------------------------------------
//  + BUILD_LOCATION
//    + ParaView C/C++ library location
//      - SELF_DIR
//    + ParaView Python modules
//      - SELF_DIR/../lib/site-packages    (when CMAKE_INTDIR is not defined).
//          OR
//      - SELF_DIR/../../lib/site-packages (when CMAKE_INTDIR is defined).
//  + INSTALL_LOCATION
//    + ParaView C/C++ library location
//      - SELF_DIR
//      - SELF_DIR/../lib/paraview-<major>.<minor>/
//    + ParaView Python modules
//      - SELF_DIR/Lib
//      - SELF_DIR/Lib/site-packages
//    + VTK Python Module libraries
//      - SELF_DIR/../lib/paraview-<major>.<minor>/site-packages/vtk
//===========================================================================
void vtkPythonAppInitPrependPathWindows(const std::string& SELF_DIR)
{
  // For use in MS VS IDE builds we need to account for selection of
  // build configuration in the IDE. CMAKE_INTDIR is how we know which
  // configuration user has selected. It will be one of Debug, Release,
  // ... etc. With in VS builds SELF_DIR will be set like
  // "builddir/bin/CMAKE_INTDIR". PYHTONPATH should have SELF_DIR and
  // "builddir/lib/CMAKE_INTDIR"
  std::string build_dir_site_packages;
#if defined(CMAKE_INTDIR)
  build_dir_site_packages = SELF_DIR + "/../../lib/site-packages";
#else
  build_dir_site_packages = SELF_DIR + "/../lib/site-packages";
#endif
  bool is_build_dir = vtkPSystemTools::FileExists(build_dir_site_packages.c_str());
  if (is_build_dir)
  {
    vtkPythonAppInitPrependPythonPath(SELF_DIR);
#if defined(CMAKE_INTDIR)
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/../../lib/" + std::string(CMAKE_INTDIR));
#else
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/../lib");
#endif
    vtkPythonAppInitPrependPythonPath(build_dir_site_packages);
  }
  else
  {
    vtkPythonAppInitPrependPythonPath(SELF_DIR);
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "Lib");
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "Lib/site-packages");
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/../lib/paraview-" PARAVIEW_VERSION);
    vtkPythonAppInitPrependPythonPath(
      SELF_DIR + "/../lib/paraview-" PARAVIEW_VERSION "/site-packages");
    // BUG #14263 happened with windows installed versions too. This addresses
    // that problem.
    vtkPythonAppInitPrependPythonPath(
      SELF_DIR + "/../lib/paraview-" PARAVIEW_VERSION "/site-packages/vtk");
  }
}
#elif defined(__APPLE__)
//===========================================================================
// OsX
// Key:
//    - SELF_DIR: directory containing the pvserver/pvpython/paraview
//      executables. This is different for app and non-app executables.
//---------------------------------------------------------------------------
//  + BUILD_LOCATION
//    - LIB_DIR
//      - SELF_DIR/../lib       (when executable is not APP, e.g pvpython)
//          OR
//      - SELF_DIR/../../../../lib (when executable is APP, e.g. paraview)
//    + ParaView C/C++ library location
//      - LIB_DIR
//    + ParaView Python modules
//      - LIB_DIR/site-packages
//  + INSTALL_LOCATION (APP)
//    - APP_ROOT
//      - SELF_DIR/../..        (this is same for paraview and pvpython)
//    - ParaView C/C++ library location
//      - APP_ROOT/Contents/Libraries/
//    - ParaView Python modules
//      - APP_ROOT/Contents/Python
//  + INSTALL_LOCATION (UNIX STYLE)
//    + SELF_DIR is "bin"
//    + ParaView C/C++ library location
//      - SELF_DIR/../lib/paraview-<major>.<minor>
//    + ParaView Python modules
//      - SELF_DIR/../lib/paraview-<major>.<minor>/site-packages
//    + VTK Python Module libraries
//      - SELF_DIR/../lib/paraview-<major>.<minor>/site-packages/vtk
//===========================================================================
void vtkPythonAppInitPrependPathOsX(const std::string& SELF_DIR)
{
  bool is_app = false;
  {
    // if SELF_DIR/../ is named "Contents", we are in an App.
    std::string contents_dir = vtkPSystemTools::CollapseFullPath((SELF_DIR + "/..").c_str());
    is_app = (vtksys::SystemTools::GetFilenameName(contents_dir) == "Contents");
  }

  std::string lib_dir =
    (is_app == false) ? (SELF_DIR + "/../lib") : (SELF_DIR + "/../../../../lib");
  lib_dir = vtkPSystemTools::CollapseFullPath(lib_dir.c_str());

  std::string cmakeconfig = (is_app == false) ? (SELF_DIR + "/../ParaViewConfig.cmake")
                                              : (SELF_DIR + "/../../../../ParaViewConfig.cmake");
  cmakeconfig = vtkPSystemTools::CollapseFullPath(cmakeconfig.c_str());

  bool is_build_dir = vtkPSystemTools::FileExists(cmakeconfig.c_str());

  // when we install on OsX using unix-style the test for is_build_dir is
  // valid for install dir too. So we do an extra check.
  bool is_unix_style_install =
    vtkPSystemTools::FileExists((lib_dir + "/paraview-" PARAVIEW_VERSION).c_str());
  if (is_build_dir)
  {
    if (is_unix_style_install)
    {
      lib_dir = lib_dir + "/paraview-" PARAVIEW_VERSION;
      vtkPythonAppInitPrependPythonPath(lib_dir);
      vtkPythonAppInitPrependPythonPath(lib_dir + "/site-packages");
      // site-packages/vtk needs to be added so the Python wrapped VTK modules
      // can be loaded from paraview e.g. import vtkCommonCorePython can work
      // (BUG #14263).
      vtkPythonAppInitPrependPythonPath(lib_dir + "/site-packages/vtk");
    }
    else // App bundle in build dir
    {
      vtkPythonAppInitPrependPythonPath(lib_dir);
      vtkPythonAppInitPrependPythonPath(lib_dir + "/site-packages");
    }
  }
  else
  {
    if (is_app)
    {
      std::string app_root = SELF_DIR + "/../..";
      app_root = vtkPSystemTools::CollapseFullPath(app_root.c_str());
      vtkPythonAppInitPrependPythonPath(app_root + "/Contents/Libraries");
      vtkPythonAppInitPrependPythonPath(app_root + "/Contents/Python");
    }
    else
    {
      vtkGenericWarningMacro("Non-app bundle in install directory not supported");
    }
  }
}
#else
void vtkPythonAppInitPrependPathLinux(const std::string& SELF_DIR);
//===========================================================================
// Linux/UNIX (not OsX)
// Key:
//    - SELF_DIR: directory containing the pvserver/pvpython/paraview
//      executables. For installed locations, this corresponds to the "real"
//      executable, not the shared-forwarded executable (if applicable).
//---------------------------------------------------------------------------
// + BUILD_LOCATION
//    + ParaView C/C++ library location
//      - SELF_DIR/../lib
//    + ParaView Python modules
//      - SELF_DIR/../lib/site-packages
//  + INSTALL_LOCATION (shared builds with shared forwarding)
//    + ParaView C/C++ library location
//      - SELF_DIR
//    + ParaView Python modules
//      - SELF_DIR/site-packages
//    + VTK Python Module libraries
//      - SELF_DIR/site-packages/vtk
//  + INSTALL_LOCATION (static builds)
//    + ParaView C/C++ library location
//      - (not applicable)
//    + ParaView Python modules
//      - SELF_DIR/../lib/paraview-<version>/site-packages
//    + VTK Python Module libraries
//      - SELF_DIR/../lib/paraview-<version>/site-packages/vtk
void vtkPythonAppInitPrependPathLinux(const std::string& SELF_DIR)
{
  // Determine if running from build or install dir.
  //    If SELF_DIR/../ParaViewConfig.cmake, it must be running from the build
  //    directory.
  bool is_build_dir = vtkPSystemTools::FileExists((SELF_DIR + "/../ParaViewConfig.cmake").c_str());
  if (is_build_dir)
  {
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/../lib");
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/../lib/site-packages");
    return;
  }

  // We're running from installed directory. We could be either a shared build
  // or a static build.
  bool using_shared_libs = false;
#ifdef BUILD_SHARED_LIBS
  using_shared_libs = true;
#endif
  if (using_shared_libs)
  {
    vtkPythonAppInitPrependPythonPath(SELF_DIR);
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/site-packages");
    // site-packages/vtk needs to be added so the Python wrapped VTK modules
    // can be loaded from paraview e.g. import vtkCommonCorePython can work
    // (BUG #14263).
    vtkPythonAppInitPrependPythonPath(SELF_DIR + "/site-packages/vtk");
  }
  else
  {
    vtkPythonAppInitPrependPythonPath(
      SELF_DIR + "/../lib/paraview-" PARAVIEW_VERSION "/site-packages");
    vtkPythonAppInitPrependPythonPath(
      SELF_DIR + "/../lib/paraview-" PARAVIEW_VERSION "/site-packages/vtk");
  }
}
//===========================================================================
#endif
#endif // ifndef PARAVIEW_FREEZE_PYTHON
}

#endif
// VTK-HeaderTest-Exclude: vtkProcessModuleInitializePython.h
