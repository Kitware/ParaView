#include "vtkKWWidgetsTourExample.h"
#include "vtkKWApplication.h"
#include <vtksys/SystemTools.hxx>

extern "C" int Kwwidgetstourexamplelib_Init(Tcl_Interp *interp);
int my_main(int argc, char *argv[])
{
  // Initialize Tcl

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
  if (!interp)
    {
    cerr << "Error: InitializeTcl failed" << endl ;
    return 1;
    }

  // Initialize our Tcl library (i.e. our classes wrapped in Tcl)

  Kwwidgetstourexamplelib_Init(interp);

  // Create an application object, then create an example object
  // and let it run the demo

  vtkKWApplication *app = vtkKWApplication::New();
  
  vtkKWWidgetsTourExample *example = vtkKWWidgetsTourExample::New();
  example->SetApplication(app);

  int res = example->Run(argc, argv);

  example->Delete();
  app->Delete();

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();

  return res;
}

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
#ifdef _MSC_VER
  // See if there are any mem leaks at exit time on Windows:
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif  //  _MSC_VER

  int argc;
  char **argv;
  vtksys::SystemTools::ConvertWindowsCommandLineToUnixArguments(
    lpCmdLine, &argc, &argv);
  int ret = my_main(argc, argv);
  for (int i = 0; i < argc; i++) { delete [] argv[i]; }
  delete [] argv;
  return ret;
}
#else
int main(int argc, char *argv[])
{
  return my_main(argc, argv);
}
#endif
