#include "vtkSetGet.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include <vtksys/SystemInformation.hxx>

#if defined(TEST_MPI_CAPS)
# include "vtkMPI.h"
#endif

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

#if defined(TEST_MPI_CAPS)
  // info about the MPI libraru
  int major=-1, minor=-1;
#if defined(MPI_VERSION)
  major = MPI_VERSION;
#endif
#if defined(MPI_SUBVERSION)
  minor = MPI_SUBVERSION;
#endif
  //MPI_Get_version(&major, &minor);

#if defined(MPI_VERSION) && (MPI_VERSION >= 3)
  char libVer[MPI_MAX_LIBRARY_VERSION_STRING] = {'\0'};
  int libVerLen = MPI_MAX_LIBRARY_VERSION_STRING;
  MPI_Get_library_version(libVer, &libVerLen);
  libVer[libVerLen] = '\0';
#else
  const char *libVer = "Unknown";
#endif
#endif

  // info about the Open GL
  vtkRenderWindow *rwin = vtkRenderWindow::New();
  rwin->Render();

  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(rwin);

  if (!context)
    {
    vtkGenericWarningMacro(
      << "ERROR: Implement support for" << rwin->GetClassName());
    return 1;
    }

  vtkOpenGLExtensionManager *extensions = context->GetExtensionManager();

  // info about the host
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
    << "Version = " << major << "." << minor << endl
    << "Library Version = " << libVer << endl
    << endl
#endif
    << "OpenGL:" << endl
    << "DriverGLVersion = " << extensions->GetDriverGLVersion() << endl
    << "DriverGLVendor = " << extensions->GetDriverGLVendor() << endl
    << "DriverGLRenderer = " << extensions->GetDriverGLRenderer() << endl
    << "DriverGLVersionMajor = " << extensions->GetDriverGLVersionMajor() << endl
    << "DriverGLVersionMinor = " << extensions->GetDriverGLVersionMinor() << endl
    << "DriverGLVersionPatch = " << extensions->GetDriverGLVersionPatch() << endl
    << "DriverVersionMajor = " << extensions->GetDriverVersionMajor() << endl
    << "DriverVersionMinor = " << extensions->GetDriverVersionMinor() << endl
    << "DriverVersionPatch = " << extensions->GetDriverVersionPatch() << endl
    << "DriverIsATI = " << extensions->DriverIsATI() << endl
    << "DriverIsNvidia = " << extensions->DriverIsNvidia() << endl
    << "DriverIsIntel = " << extensions->DriverIsIntel() << endl
    << "DriverIsMesa = " << extensions->DriverIsMesa() << endl
    << "DriverGLRendererIsOSMesa = " << extensions->DriverGLRendererIsOSMesa() << endl
    << "DriverIsMicrosoft = " << extensions->DriverIsMicrosoft() << endl
    << "Extensions = " << extensions->GetExtensionsString() << endl
    << endl;

  rwin->Delete();

  // always pass
  return 0;
}
