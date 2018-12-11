#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#endif

#include "vtk_glew.h"

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkSetGet.h"

#if VTK_MODULE_ENABLE_VTK_Python
#include "patchlevel.h"
#endif

#include <sstream>
#include <string>
#include <vtksys/SystemInformation.hxx>

using std::string;
using std::ostringstream;

// Description:
// Get python version
#if VTK_MODULE_ENABLE_VTK_Python
string GetPythonVersion()
{
  ostringstream oss;
#if defined(PY_VERSION)
  oss << PY_VERSION;
#else
  oss << "unknown";
#endif
  return oss.str();
}
#endif

// Description:
// Get the version of the standard implemented by this
// MPI
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
string GetMPIVersion()
{
  ostringstream oss;
  int major = -1, minor = -1;
#if defined(MPI_VERSION)
  major = MPI_VERSION;
#endif
#if defined(MPI_SUBVERSION)
  minor = MPI_SUBVERSION;
#endif
  // MPI_Get_version(&major, &minor);
  oss << major << "." << minor;
  return oss.str();
}
#endif

// Description:
// Get the implementor name and release info
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
string GetMPILibraryVersion()
{
  ostringstream oss;
#if defined(MPI_VERSION) && (MPI_VERSION >= 3)
  char libVer[MPI_MAX_LIBRARY_VERSION_STRING] = { '\0' };
  int libVerLen = MPI_MAX_LIBRARY_VERSION_STRING;
  MPI_Get_library_version(libVer, &libVerLen);
  libVer[libVerLen] = '\0';
  oss << libVer;
#else
// Open MPI
#if defined(OPEN_MPI)
  oss << "Open MPI";
#if defined(OMPI_MAJOR_VERSION)
  oss << " " << OMPI_MAJOR_VERSION;
#endif
#if defined(OMPI_MINOR_VERSION)
  oss << "." << OMPI_MINOR_VERSION;
#endif
#if defined(OMPI_RELEASE_VERSION)
  oss << "." << OMPI_RELEASE_VERSION;
#endif
// MPICH
#elif defined(MPICH2)
  oss << "MPICH2";
#if defined(MPICH2_VERSION)
  oss << " " << MPICH2_VERSION;
#endif
#elif defined(MSMPI_VER)
  oss << "Microsoft MPI " << MSMPI_VER;
#else
  oss << "unknown";
#endif
#endif
  return oss.str();
}
#endif

#define safes(arg) (arg ? ((const char*)arg) : "<null>")

string GetOpenGLInfo()
{
  ostringstream oss;
  vtkNew<vtkRenderWindow> rwin;
  rwin->SetOffScreenRendering(1);
  rwin->Render();
  oss << "DriverGLVersion = " << safes(glGetString(GL_VERSION)) << endl
      << "DriverGLVendor = " << safes(glGetString(GL_VENDOR)) << endl
      << "DriverGLRenderer = " << safes(glGetString(GL_RENDERER)) << endl;
  return oss.str();
}

int TestSystemCaps(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // for info about the host
  vtksys::SystemInformation sysinfo;
  sysinfo.RunCPUCheck();
  sysinfo.RunOSCheck();

  // make the report
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl
       << endl
       << "Host System:" << endl
       << "OS = " << sysinfo.GetOSDescription() << endl
       << "CPU = " << sysinfo.GetCPUDescription() << endl
       << "RAM = " << sysinfo.GetMemoryDescription() << endl
       << endl
#if defined(TEST_MPI_CAPS)
       << "MPI:" << endl
       << "Version = " << GetMPIVersion() << endl
       << "Library = " << GetMPILibraryVersion() << endl
       << endl
#endif
#if defined(TEST_PY_CAPS)
       << "Python:" << endl
       << "Version = " << GetPythonVersion() << endl
       << endl
#endif
       << "OpenGL:" << endl
       << GetOpenGLInfo() << endl;

  // always pass
  return 0;
}
