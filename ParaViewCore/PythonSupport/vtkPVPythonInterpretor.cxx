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

#include "vtkPython.h"
#include "vtkPVPythonInterpretor.h"


#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPythonAppInitConfigure.h"
#include "vtkStdString.h"
#include "vtkWindows.h"
#include "pvpythonmodules.h"

#include <vtksys/SystemTools.hxx>
#include <algorithm>
#include <string>

#include <signal.h>  // for signal

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

#include "vtkPVPythonInterpretorWrapper.h"
//----------------------------------------------------------------------------
static void vtkPythonAppInitPrependPythonPath(const char* dir)
{
  // Convert slashes for this platform.
  std::string out_dir = dir ? dir : "";
#if defined(_WIN32) && !defined(__CYGWIN__)
  std::replace(out_dir.begin(), out_dir.end(), '/', '\\');
#endif

  // Append the path to the python sys.path object.
  PyObject* path = PySys_GetObject(const_cast<char*>("path"));
  PyObject* newpath = PyString_FromString(out_dir.c_str());
  PyList_Insert(path, 0, newpath);
  Py_DECREF(newpath);
}

//----------------------------------------------------------------------------
static bool vtkPythonAppInitPrependPath2(const std::string& prefix,
  const std::string& path)
{
  std::string package_dir;
  package_dir = prefix + "/../" + path;
  package_dir = vtksys::SystemTools::CollapseFullPath(package_dir.c_str());
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
static void vtkPythonAppInitPrependPath(const char* self_dir)
{
  // Try to put the VTK python module location in sys.path.
  std::string pkg_prefix = self_dir;
#if defined(CMAKE_INTDIR)
  pkg_prefix += "/..";
#endif
  vtkPythonAppInitPrependPath2(pkg_prefix, "Utilities/mpi4py");
  if (vtkPythonAppInitPrependPath2(pkg_prefix, "Utilities/VTKPythonWrapping/site-packages"))
    {
    // This executable is running from the build tree.  Prepend the
    // library directory to the search path.
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
      "/../Python/paraview", // MacOS bundle
      "/../lib/paraview-" PARAVIEW_VERSION "/paraview",
      "/../../lib/paraview-" PARAVIEW_VERSION "/paraview",
      "/lib/python" VTK_PYTHON_VERSION "/site-packages/paraview", // UNIX --prefix
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
    std::string lib_dir = std::string(prefix + "/../lib/paraview-" + PARAVIEW_VERSION);
    lib_dir = vtksys::SystemTools::CollapseFullPath( lib_dir.c_str());
    vtkPythonAppInitPrependPythonPath(lib_dir.c_str());
#endif

    // These two directories should be all that is necessary when running the
    // python interpreter in a build tree.
    vtkPythonAppInitPrependPythonPath(PV_PYTHON_PACKAGE_DIR "/site-packages");
    vtkPythonAppInitPrependPythonPath(VTK_PYTHON_LIBRARY_DIR_BUILD);

#if defined(__APPLE__)
    // On OS X distributions, the libraries are in a different directory
    // than the module. They are in a place relative to the executable.
    std::string libs_dir = std::string(self_dir) + "/../Libraries";
    libs_dir = vtksys::SystemTools::CollapseFullPath(libs_dir.c_str());
    if(vtksys::SystemTools::FileIsDirectory(libs_dir.c_str()))
      {
      vtkPythonAppInitPrependPythonPath(libs_dir.c_str());
      }
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

struct vtkPythonMessage
{
  vtkStdString Message;
  bool IsError;
};

//-----------------------------------------------------------------------------
class vtkPVPythonInterpretorInternal
{
public:
  PyThreadState* Interpretor;
  PyThreadState* PreviousInterpretor; // save when MakeCurrent is called.

  std::vector<vtkPythonMessage> Messages;

  vtkPVPythonInterpretorInternal()
    {
    this->Interpretor = 0;
    this->PreviousInterpretor = 0;
    }

  ~vtkPVPythonInterpretorInternal()
    {
    if (this->Interpretor)
      {
      this->MakeCurrent();
      Py_EndInterpreter(this->Interpretor);
      this->ReleaseControl();
      this->Interpretor = 0;
      }
    }

  void MakeCurrent()
    {
    if (this->PreviousInterpretor)
      {
      vtkGenericWarningMacro("MakeCurrent cannot be called recursively."
        "Please call ReleaseControl() befor calling MakeCurrent().");
      return;
      }

    if (this->Interpretor)
      {
      this->PreviousInterpretor = PyThreadState_Swap(this->Interpretor);
      }
    }

  void ReleaseControl()
    {
    PyThreadState_Swap(this->PreviousInterpretor);
    this->PreviousInterpretor = 0;
    }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPythonInterpretor);

//-----------------------------------------------------------------------------
vtkPVPythonInterpretor::vtkPVPythonInterpretor()
{
  this->ActiveSessionObserverAttached = false;
  this->Internal = new vtkPVPythonInterpretorInternal();
  this->ExecutablePath = 0;
  this->CaptureStreams = false;
}

//-----------------------------------------------------------------------------
vtkPVPythonInterpretor::~vtkPVPythonInterpretor()
{
  this->DetachActiveSessionObserver();
  delete this->Internal;
  this->SetExecutablePath(0);
}

//-----------------------------------------------------------------------------
vtkStdString vtkPVPythonInterpretor::GetInputLine()
{
  vtkStdString ret;
  this->InvokeEvent(vtkCommand::UpdateEvent, &ret);
  return ret;
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::DumpError(const char* str)
{
  vtkPythonMessage msg;
  msg.Message = str;
  msg.IsError = true;
  if (msg.Message.size() > 0)
    {
    // add new entry only if type changed.
    if (this->Internal->Messages.size() > 0 &&
      this->Internal->Messages.back().IsError)
      {
      this->Internal->Messages.back().Message += str;
      }
    else
      {
      this->Internal->Messages.push_back(msg);
      }
    this->InvokeEvent(vtkCommand::ErrorEvent, const_cast<char*>(str));
    }
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::DumpOutput(const char* str)
{
  vtkPythonMessage msg;
  msg.Message = str;
  msg.IsError = false;
  if (msg.Message.size() > 0)
    {
    // add new entry only if type changed.
    if (this->Internal->Messages.size() > 0 &&
      !this->Internal->Messages.back().IsError)
      {
      this->Internal->Messages.back().Message += str;
      }
    else
      {
      this->Internal->Messages.push_back(msg);
      }
    this->InvokeEvent(vtkCommand::WarningEvent, const_cast<char*>(str));
    }
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::FlushMessages()
{
  std::vector<vtkPythonMessage>::iterator iter =
    this->Internal->Messages.begin();

  for (; iter != this->Internal->Messages.end(); ++iter)
    {
    if (iter->IsError)
      {
      vtkOutputWindowDisplayErrorText(iter->Message.c_str());
      }
    else
      {
      vtkOutputWindowDisplayText(iter->Message.c_str());
      }
    }
  this->ClearMessages();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::ClearMessages()
{
  this->Internal->Messages.clear();
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
  const char* exe_str =  this->ExecutablePath;
  if (!exe_str)
    {
    PyObject* executable = PySys_GetObject(const_cast<char*>("executable"));
    exe_str = PyString_AsString(executable);
    }
  if (exe_str)
    {
    // Use the executable location to try to set sys.path to include
    // the VTK python modules.
    std::string self_dir = vtksys::SystemTools::GetFilenamePath(exe_str);
    vtkPythonAppInitPrependPath(self_dir.c_str());
    }

  if (this->CaptureStreams)
    {
    // HACK: Calling PyRun_SimpleString for the first time for some reason results in
    // a "\n" message being generated which is causing the error dialog to
    // popup. So we flush that message out of the system before setting up the
    // callbacks.
    // The cast is necessary because PyRun_SimpleString() hasn't always been
    // const-correct.
    PyRun_SimpleString(const_cast<char*>(""));
    vtkPVPythonInterpretorWrapper* wrapperOut = vtkWrapInterpretor(this);
    wrapperOut->DumpToError = false;

    vtkPVPythonInterpretorWrapper* wrapperErr = vtkWrapInterpretor(this);
    wrapperErr->DumpToError = true;

    // Redirect Python's stdout and stderr and stdin
    PySys_SetObject(const_cast<char*>("stdout"),
      reinterpret_cast<PyObject*>(wrapperOut));

    PySys_SetObject(const_cast<char*>("stderr"),
      reinterpret_cast<PyObject*>(wrapperErr));

    PySys_SetObject(const_cast<char*>("stdin"),
      reinterpret_cast<PyObject*>(wrapperErr));

    Py_DECREF(wrapperOut);
    Py_DECREF(wrapperErr);
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

  this->SetExecutablePath(argv[0]);
  if (!Py_IsInitialized())
    {
    // Set the program name, so that we can ask python to provide us
    // full path.
    Py_SetProgramName(argv[0]);
    Py_Initialize();

#ifdef SIGINT
    signal(SIGINT, SIG_DFL);
#endif
    }

  // Py_NewInterpreter() should not be called when a sub-interpretor is active.
  // So ensure that the sub-interpretor is not active by doing the following.
  PyThreadState* cur = PyThreadState_Swap(0);
  this->Internal->Interpretor = Py_NewInterpreter();
  this->Internal->MakeCurrent();
  this->InitializeInternal();
  this->Internal->ReleaseControl();
  PyThreadState_Swap(cur);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::AddPythonPath(const char* path)
{
  if (!this->Internal->Interpretor)
    {
    vtkErrorMacro("SubInterpretor not initialized. Call InitializeSubInterpretor().");
    return;
    }

  this->MakeCurrent();
  this->AddPythonPathInternal(path);
  this->ReleaseControl();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::AddPythonPathInternal(const char* path)
{
  vtkPythonAppInitPrependPythonPath(path);
}

//-----------------------------------------------------------------------------
int vtkPVPythonInterpretor::PyMain(int argc, char** argv)
{
  // Set the program name, so that we can ask python to provide us
  // full path.
  Py_SetProgramName(argv[0]);

  // initialize the statically linked modules
  CMakeLoadAllPythonModules();

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
void vtkPVPythonInterpretor::ReleaseControl()
{
  this->Internal->ReleaseControl();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::RunSimpleString(const char* const script)
{
  this->MakeCurrent();

  // The embedded python interpreter cannot handle DOS line-endings, see
  // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
  std::string buffer = script ? script : "";
  buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());

  // The cast is necessary because PyRun_SimpleString() hasn't always been const-correct
  PyRun_SimpleString(const_cast<char*>(buffer.c_str()));

  this->ReleaseControl();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::RunSimpleFile(const char* const filename)
{
  this->MakeCurrent();

  FILE* fp = fopen(filename, "r");
  if (!fp)
    {
    vtkErrorMacro("Failed to open file " << filename);
    return;
    }

  // The cast is necessary because PyRun_SimpleFile() hasn't always been const-correct
  PyRun_SimpleFile(fp, const_cast<char*>(filename));

  fclose(fp);
  this->ReleaseControl();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::ExecuteInitFromGUI()
{
  const char* initStr = 
    "import paraview\n"
    "paraview.compatibility.major = 3\n"
    "paraview.compatibility.minor = 5\n"
    "from paraview import servermanager\n"
    "servermanager.InitFromGUI()\n"
    "from paraview.simple import *\n"
    "active_objects.view = servermanager.GetRenderView()\n"
    "";
  this->ActiveSessionObserverAttached = true;
  this->RunSimpleString(initStr);
  this->FlushMessages();
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::DetachActiveSessionObserver()
{
if(this->ActiveSessionObserverAttached)
  {
  this->RunSimpleString("paraview.simple.active_session_observer = None\n");
  this->FlushMessages();
  }
}

//-----------------------------------------------------------------------------
void vtkPVPythonInterpretor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
