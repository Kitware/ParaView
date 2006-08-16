/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInterpretor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPythonInterpretor.h"

#include "vtkObjectFactory.h"
#include "vtkPythonAppInitConfigure.h"
#include "vtkPython.h"
#include "vtkWindows.h"

#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
#if defined(CMAKE_INTDIR)
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD "/" CMAKE_INTDIR
#else
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD
#endif

/* The maximum length of a file name.  */
#if defined(PATH_MAX)
# define VTK_PYTHON_MAXPATH PATH_MAX
#elif defined(MAXPATHLEN)
# define VTK_PYTHON_MAXPATH MAXPATHLEN
#else
# define VTK_PYTHON_MAXPATH 16384
#endif

/* Python major.minor version string.  */
#define VTK_PYTHON_TO_STRING(x) VTK_PYTHON_TO_STRING0(x)
#define VTK_PYTHON_TO_STRING0(x) VTK_PYTHON_TO_STRING1(x)
#define VTK_PYTHON_TO_STRING1(x) #x
#define VTK_PYTHON_VERSION VTK_PYTHON_TO_STRING(PY_MAJOR_VERSION.PY_MINOR_VERSION)



extern "C" {
  extern DL_IMPORT(int) Py_Main(int, char **);
}

//----------------------------------------------------------------------------
static void vtkPythonAppInitPrependPythonPath(const char* dir)
{
  // Convert slashes for this platform.
  vtkstd::string out_dir = dir;
#if defined(_WIN32) && !defined(__CYGWIN__)
  for(vtkstd::string::size_type i = 0; i < out_dir.length(); ++i)
    {
    if(out_dir[i] == '/')
      {
      out_dir[i] = '\\';
      }
    }
#endif

  // Append the path to the python sys.path object.
  char tmpPath[] = "path";
  PyObject* path = PySys_GetObject(tmpPath);
  PyObject* newpath;
  newpath = PyString_FromString(out_dir.c_str());
  PyList_Insert(path, 0, newpath);
  Py_DECREF(newpath);
}

//----------------------------------------------------------------------------
static void vtkPythonAppInitPrependPath(const char* self_dir)
{
  // Try to put the VTK python module location in sys.path.
  vtkstd::string package_dir = self_dir;
#if defined(CMAKE_INTDIR)
  package_dir += "/..";
#endif
  package_dir += "/../Utilities/VTKPythonWrapping";
  package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
  if(vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
    {
    // This executable is running from the build tree.  Prepend the
    // library directory and package directory to the search path.
    vtkPythonAppInitPrependPythonPath(package_dir.c_str());
    vtkPythonAppInitPrependPythonPath(VTK_PYTHON_LIBRARY_DIR);
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
      "/../lib/paraview-" PARAVIEW_VERSION "/paraview",
      "/../../lib/paraview-" PARAVIEW_VERSION "/paraview",
      "/lib/python" VTK_PYTHON_VERSION "/site-packages/paraview", // UNIX --prefix
      "/lib/python/paraview", // UNIX --home
      "/Lib/site-packages/paraview", "/Lib/paraview", // Windows
      "/site-packages/paraview", "/paraview", // Windows
      0
    };
    vtkstd::string prefix = vtksys::SystemTools::GetFilenamePath(self_dir);
    for(const char** dir = inst_dirs; *dir; ++dir)
      {
      package_dir = prefix;
      package_dir += *dir;
      package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
      if(vtksys::SystemTools::FileIsDirectory(package_dir.c_str()))
        {
        // We found the modules.  Add the location to sys.path, but
        // without the "/vtk" suffix.
        vtkstd::string path_dir =
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

//-----------------------------------------------------------------------------
class vtkPVPythonInterpretorInternal
{
public:
  PyThreadState* Interpretor;

  vtkPVPythonInterpretorInternal()
    {
    this->Interpretor = 0;
    }
  ~vtkPVPythonInterpretorInternal()
    {
    if (this->Interpretor)
      {
      this->MakeCurrent();
      Py_EndInterpreter(this->Interpretor);
      this->Interpretor= 0;
      this->MakeCurrent();
      }
    }
  void MakeCurrent()
    {
    PyThreadState_Swap(this->Interpretor);
    }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPythonInterpretor);
vtkCxxRevisionMacro(vtkPVPythonInterpretor, "1.1");

//-----------------------------------------------------------------------------
vtkPVPythonInterpretor::vtkPVPythonInterpretor()
{
  this->Internal = new vtkPVPythonInterpretorInternal();
}

//-----------------------------------------------------------------------------
vtkPVPythonInterpretor::~vtkPVPythonInterpretor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::InitializeInternal()
{
  // The following code will hack in the path for running VTK/Python
  // from the build tree. Do not try this at home. We are
  // professionals.

  // Compute the directory containing this executable.  The python
  // sys.executable variable contains the full path to the interpreter
  // executable.
  char tmpExe[] = "executable";
  PyObject* executable = PySys_GetObject(tmpExe);
  if(const char* exe_str = PyString_AsString(executable))
    {
    // Use the executable location to try to set sys.path to include
    // the VTK python modules.
    vtkstd::string self_dir = vtksys::SystemTools::GetFilenamePath(exe_str);
    vtkPythonAppInitPrependPath(self_dir.c_str());
    }
}

//-----------------------------------------------------------------------------
int vtkPVPythonInterpretor::InitializeSubInterpretor(int vtkNotUsed(argc), 
  char** argv)
{
  if (this->Internal->Interpretor)
    {
    vtkErrorMacro("SubInterpretor already initialized.");
    return 0;
    }

  if (!Py_IsInitialized())
    {
    // Set the program name, so that we can ask python to provide us
    // full path.
    Py_SetProgramName(argv[0]);
    Py_Initialize();
    }

  this->Internal->Interpretor = Py_NewInterpreter();
  this->Internal->MakeCurrent();
  this->InitializeInternal();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVPythonInterpretor::PyMain(int argc, char** argv)
{
  // Set the program name, so that we can ask python to provide us
  // full path.
  Py_SetProgramName(argv[0]);

  // Initialize interpreter.
  Py_Initialize();

  this->InitializeInternal();
  return Py_Main(argc, argv);
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::MakeCurrent()
{
  this->Internal->MakeCurrent();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
