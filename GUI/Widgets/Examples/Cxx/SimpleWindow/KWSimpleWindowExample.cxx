#include "vtkKWApplication.h"
#include "vtkKWWindowBase.h"
#include "vtkKWLabel.h"
#include "vtkKWFrame.h"

#include <vtksys/SystemTools.hxx>

int my_main(int argc, char *argv[])
{
  // Initialize Tcl

  Tcl_Interp *res = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
  if (!res)
    {
    cerr << "Error: InitializeTcl failed" << endl ;
    return 1;
    }

  // Create the application
  // Restore the settings that have been saved to the registry, like
  // the geometry of the user interface so far.

  vtkKWApplication *app = vtkKWApplication::New();
  app->RestoreApplicationSettingsFromRegistry();
  app->SetName("KWSimpleWindowExample");

  // Set a help link. Can be a remote link (URL), or a local file

  // vtksys::SystemTools::GetFilenamePath(__FILE__) + "/help.html";
  app->SetHelpDialogStartingPage("http://www.kitware.com");

  // Add a window
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindowBase *win = vtkKWWindowBase::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create(app, NULL);

  // Add a label, attach it to the view frame, and pack
  
  vtkKWLabel *hello_label = vtkKWLabel::New();
  hello_label->SetParent(win->GetViewFrame());
  hello_label->Create(app, NULL);
  hello_label->SetText("Hello, World!");
  app->Script("pack %s -side left -anchor c -expand y", 
              hello_label->GetWidgetName());
  hello_label->Delete();

  // Start the application

  app->Start(argc, argv);
  int ret = app->GetExitStatus();

  // Deallocate and exit

  win->Delete();
  app->Delete();
  
  return ret;
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
