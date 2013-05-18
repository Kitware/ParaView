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
#ifndef __vtkProcessModulePythonInitializePython_h
#define __vtkProcessModulePythonInitializePython_h


#include <vtksys/SystemTools.hxx>
#include <algorithm>
#include <string>
#include "vtkPythonInterpreter.h"

/* The maximum length of a file name.  */
#if defined(PATH_MAX)
# define VTK_PYTHON_MAXPATH PATH_MAX
#elif defined(MAXPATHLEN)
# define VTK_PYTHON_MAXPATH MAXPATHLEN
#else
# define VTK_PYTHON_MAXPATH 16384
#endif
namespace
{
  // This is code that used to reside in vtkPVPythonInterpretor. We should
  // sanitize and clean it up. Since I don't want to muck with the search paths
  // close to a release, I am leaving this untouched.

  // THIS NEEDS TO BE CLEANED LIKE THE PLAGUE!!!
  //----------------------------------------------------------------------------
  void vtkPythonAppInitPrependPythonPath(const char* dir)
    {
    vtkPythonInterpreter::PrependPythonPath(dir);
    }

  //----------------------------------------------------------------------------
  bool vtkPythonAppInitPrependPath2(const std::string& prefix, const std::string& path)
    {
    std::string package_dir = prefix + path;
    package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
    if (!vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
      {
      package_dir = prefix + "/../" + path;
      package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
      }

    if (!vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
      {
      // This is the right path for app bundles on OS X
      package_dir = prefix + "/../../../../" + path;
      package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
      }

    if(vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
      {
      // This executable is running from the build tree.  Prepend the
      // library directory and package directory to the search path.
      vtkPythonAppInitPrependPythonPath(package_dir.c_str());
      return true;
      }
    return false;
    }

  //----------------------------------------------------------------------------
  void vtkPythonAppInitPrependPath(const char* self_dir)
    {
    // Try to put the VTK python module location in sys.path.
    std::string pkg_prefix = self_dir;
#if defined(CMAKE_INTDIR)
    pkg_prefix += "/..";
#endif
    vtkPythonAppInitPrependPath2(pkg_prefix, "/site-packages");
    vtkPythonAppInitPrependPath2(pkg_prefix, "/lib/site-packages");
    vtkPythonAppInitPrependPath2(pkg_prefix, "/../lib/site-packages");
    vtkPythonAppInitPrependPath2(pkg_prefix, "/lib");
    vtkPythonAppInitPrependPath2(pkg_prefix, "/../lib");

    if (vtkPythonAppInitPrependPath2(pkg_prefix, "Utilities/VTKPythonWrapping/site-packages"))
      {
      // This executable is running from the build tree.  Prepend the
      // library directory to the search path.
      //vtkPythonAppInitPrependPythonPath(VTK_PYTHON_LIBRARY_DIR);
      }
    else
      {
      // This executable is running from an install tree.  Check for
      // possible VTK python module locations.  See
      // http://python.org/doc/2.4.1/inst/alt-install-windows.html for
      // information about possible install locations.  If the user
      // changes the prefix to something other than VTK's prefix or
      // python's native prefix then he/she will have to get the
      // packages in sys.path himself/herself.
      const char* inst_dirs[] = {
        "/paraview",
        "/../Python/paraview", // MacOS bundle
        "/../lib/paraview-" PARAVIEW_VERSION "/paraview",
        "/../../lib/paraview-" PARAVIEW_VERSION "/paraview",
        "/lib/python/paraview", // UNIX --home
        "/Lib/site-packages/paraview", "/Lib/paraview", // Windows
        "/site-packages/paraview", "/paraview", // Windows
        "/../lib/paraview-" PARAVIEW_VERSION "/site-packages/paraview",
        "/../lib/paraview-" PARAVIEW_VERSION "/site-packages",
        0
      };

      std::string prefix = self_dir;
      vtkPythonAppInitPrependPythonPath(self_dir); // Propbably not needed any longer.

#if defined(WIN32)
      // for when running from installed location.
      std::string lib_dir = std::string(prefix + "/../lib/paraview-" + PARAVIEW_VERSION);
      lib_dir = vtksys::SystemTools::CollapseFullPath( lib_dir.c_str());
      vtkPythonAppInitPrependPythonPath(lib_dir.c_str());
      vtkPythonAppInitPrependPythonPath( (lib_dir + "/site-packages").c_str() );
#endif

      // These two directories should be all that is necessary when running the
      // python interpreter in a build tree.
      //vtkPythonAppInitPrependPythonPath(PV_PYTHON_PACKAGE_DIR "/site-packages");
      //vtkPythonAppInitPrependPythonPath(VTK_PYTHON_LIBRARY_DIR "/site-packages");
      //vtkPythonAppInitPrependPythonPath(VTK_PYTHON_LIBRARY_DIR);

#if defined(__APPLE__)
      // On OS X distributions, the libraries are in a different directory
      // than the module. They are in a place relative to the executable.
      std::string libs_dir = std::string(self_dir) + "/../Libraries";
      libs_dir = vtksys::SystemTools::CollapseFullPath(libs_dir.c_str());
      if(vtksys::SystemTools::FileIsDirectory(libs_dir.c_str()))
        {
        vtkPythonAppInitPrependPythonPath(libs_dir.c_str());
        }
      vtkPythonAppInitPrependPath2(prefix, "/../Python");
#endif
      for(const char** dir = inst_dirs; *dir; ++dir)
        {
        std::string package_dir;
        package_dir = prefix;
        package_dir += *dir;
        package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
        if(vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
          {
          // We found the modules.  Add the location to sys.path, but
          // without the "/vtk" suffix.
          std::string path_dir =
            vtksys::SystemTools::GetFilenamePath(package_dir);
          vtkPythonAppInitPrependPythonPath(path_dir.c_str());
          break;
          }
        }
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
      static char system_path[(VTK_PYTHON_MAXPATH+1)*10] = "PATH=";
      strcat(system_path, self_dir);
      if(char* oldpath = getenv("PATH"))
        {
        strcat(system_path, ";");
        strcat(system_path, oldpath);
        }
      putenv(system_path);
#endif
      }
    }
}

#endif
// VTK-HeaderTest-Exclude: vtkProcessModuleInitializePython.h
