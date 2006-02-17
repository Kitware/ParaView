#include "vtkActor.h"
#include "vtkKWApplication.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWSurfaceMaterialPropertyWidget.h"
#include "vtkKWWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkKWSimpleAnimationWidget.h"

#include "vtkKWWidgetsPaths.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>

int my_main(int argc, char *argv[])
{
  // Initialize Tcl

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
  if (!interp)
    {
    cerr << "Error: InitializeTcl failed" << endl ;
    return 1;
    }

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
  app->SetName("KWPolygonalObjectViewerExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    app->PromptBeforeExitOff();
    }
  app->RestoreApplicationSettingsFromRegistry();

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kwwidgets.org");

  // Add a window
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindow *win = vtkKWWindow::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create();
  win->SecondaryPanelVisibilityOff();

  // Add a render widget, attach it to the view frame, and pack
  
  vtkKWRenderWidget *rw = vtkKWRenderWidget::New();
  rw->SetParent(win->GetViewFrame());
  rw->Create();

  app->Script("pack %s -expand y -fill both -anchor c -expand y", 
              rw->GetWidgetName());

  // Create a 3D object reader

  vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();

  char data_path[2048];
  sprintf(data_path, "%s/Data/teapot.vtp", KWWidgets_EXAMPLES_DIR);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/..%s/Examples/Data/teapot.vtp",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR);
    }
  reader->SetFileName(data_path);

  // Create the mapper and actor

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Add the actor to the scene

  rw->AddViewProp(actor);
  rw->ResetCamera();

  // Create a material property editor

  vtkKWSurfaceMaterialPropertyWidget *mat_prop_widget = 
    vtkKWSurfaceMaterialPropertyWidget::New();
  mat_prop_widget->SetParent(win->GetMainPanelFrame());
  mat_prop_widget->Create();
  mat_prop_widget->SetPropertyChangedCommand(rw, "Render");
  mat_prop_widget->SetPropertyChangingCommand(rw, "Render");

  mat_prop_widget->SetProperty(actor->GetProperty());

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              mat_prop_widget->GetWidgetName());

  // Create a simple animation widget

  vtkKWFrameWithLabel *animation_frame = vtkKWFrameWithLabel::New();
  animation_frame->SetParent(win->GetMainPanelFrame());
  animation_frame->Create();
  animation_frame->SetLabelText("Movie Creator");

  app->Script("pack %s -side top -anchor nw -expand n -fill x -pady 2",
              animation_frame->GetWidgetName());

  vtkKWSimpleAnimationWidget *animation_widget = 
    vtkKWSimpleAnimationWidget::New();
  animation_widget->SetParent(animation_frame->GetFrame());
  animation_widget->Create();
  animation_widget->SetRenderWidget(rw);
  animation_widget->SetAnimationTypeToCamera();

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              animation_widget->GetWidgetName());

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

  mat_prop_widget->Delete();
  animation_frame->Delete();
  animation_widget->Delete();
  reader->Delete();
  actor->Delete();
  mapper->Delete();
  rw->Delete();
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
