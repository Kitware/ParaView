#include "vtkActor.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindowBase.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkXMLPolyDataReader.h"

#include "vtkKWWidgetsConfigure.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>

int my_main(int argc, char *argv[])
{
  // Initialize Tcl

  Tcl_Interp *res = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
  if (!res)
    {
    cerr << "Error: InitializeTcl failed" << endl ;
    return 1;
    }

  // Process some command-line arguments

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
  app->SetName("KWSimpleWindowWithRenderWidgetExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    app->PromptBeforeExitOff();
    }
  app->RestoreApplicationSettingsFromRegistry();

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kitware.com");

  // Add a window
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindowBase *win = vtkKWWindowBase::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create(app);

  // Add a render widget, attach it to the view frame, and pack
  
  vtkKWRenderWidget *rw = vtkKWRenderWidget::New();
  rw->SetParent(win->GetViewFrame());
  rw->Create(app);

  app->Script("pack %s -expand y -fill both -anchor c -expand y", 
              rw->GetWidgetName());

  // Switch to trackball style, it's nicer

  vtkInteractorStyleSwitch *istyle = vtkInteractorStyleSwitch::SafeDownCast(
    rw->GetRenderWindow()->GetInteractor()->GetInteractorStyle());
  if (istyle)
    {
    istyle->SetCurrentStyleToTrackballCamera();
    }

  // Create a 3D object reader

  vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();

  char data_path[2048];
  sprintf(data_path, "%s/Examples/Data/teapot.vtp", KWWIDGETS_SOURCE_DIR);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/../share/%s/Examples/Data/teapot.vtp",
            app->GetInstallationDirectory(), KWWIDGETS_PROJECT_NAME);
    }
  reader->SetFileName(data_path);

  // Create the mapper and actor

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Add the actor to the scene

  rw->AddProp(actor);
  rw->ResetCamera();

  // Start the application
  // If --test was provided, do not enter the event loop

  int ret = 0;
  win->Display();
  if (!option_test)
    {
    app->Start(argc, argv);
    ret = app->GetExitStatus();
    }
  win->Close();

  // Deallocate and exit

  reader->Delete();
  actor->Delete();
  mapper->Delete();
  rw->Delete();
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
