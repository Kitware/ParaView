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

  return res;
}

#ifdef _WIN32
#include <windows.h>
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
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
