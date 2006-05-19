#include "vtkKWApplication.h"
#include "vtkKWMyWindow.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>

extern "C" int Kwmedicalimageviewerexamplelib_Init(Tcl_Interp *interp);
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
  // Make sure it matches the name of the library that was created

  Kwmedicalimageviewerexamplelib_Init(interp);

  // Process some command-line arguments
  // The --test option here is used to run this example as a non-interactive 
  // test for software quality purposes. You can ignore it.

  int option_test = 0;
  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.AddArgument(
    "--test", vtksys::CommandLineArguments::NO_ARGUMENT, &option_test, "");
  args.Parse();
  
  // Create the application
  // If --test was provided, ignore all registry settings, and exit silently
  // Restore the settings that have been saved to the registry, like
  // the geometry of the user interface so far.

  vtkKWApplication *app = vtkKWApplication::New();
  app->SetName("KWMedicalImageViewerExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    app->PromptBeforeExitOff();
    }
  app->RestoreApplicationSettingsFromRegistry();

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kwwidgets.org");

  // Add our window
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWMyWindow *win = vtkKWMyWindow::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create();
  win->SecondaryPanelVisibilityOff();

  // Start the application
  // If --test was provided, do not enter the event loop and run this example
  // as a non-interactive test for software quality purposes.

  int ret = 0;
  win->Display();
  if (!option_test)
    {
    app->Start(argc, argv);
    ret = app->GetExitStatus();
    }
  win->Close();

  // Deallocate and exit

  win->Delete();
  app->Delete();
  
  return ret;
}

#if defined(_WIN32) && !defined(__CYGWIN__)
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
