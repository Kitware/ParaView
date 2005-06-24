#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindowBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"
#include "vtkKWScale.h"
#include "vtkKWTkUtilities.h"

#include "vtkKWWidgetsConfigurePaths.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/stl/string>

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
  app->SetName("KWSimpleWindowWithImageWidgetExample");
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

  // Create a volume reader

  vtkXMLImageDataReader *reader = vtkXMLImageDataReader::New();

  char data_path[2048];
  sprintf(
    data_path, "%s/Examples/Data/head100x100x47.vti", KWWIDGETS_SOURCE_DIR);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/..%s/Examples/Data/head100x100x47.vti",
            app->GetInstallationDirectory(), KW_INSTALL_SHARE_DIR);
    }
  reader->SetFileName(data_path);

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  vtkImageViewer2 *viewer = vtkImageViewer2::New();
  viewer->SetRenderWindow(rw->GetRenderWindow());
  viewer->SetRenderer(rw->GetRenderer());
  viewer->SetInput(reader->GetOutput());

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  viewer->SetupInteractor(iren);

  // Reset the window/level and the camera

  reader->Update();
  double *range = reader->GetOutput()->GetScalarRange();
  viewer->SetColorWindow(range[1] - range[0]);
  viewer->SetColorLevel(0.5 * (range[1] + range[0]));

  rw->ResetCamera();

  // Create a scale to control the slice

  vtkKWScale *slice_scale = vtkKWScale::New();
  slice_scale->SetParent(win->GetViewFrame());
  slice_scale->Create(app);
  slice_scale->DisplayEntry();
  slice_scale->DisplayEntryAndLabelOnTopOff();
  slice_scale->DisplayLabel("Slice:");
  slice_scale->SetRange(viewer->GetWholeZMin(), viewer->GetWholeZMax());
  slice_scale->SetValue(viewer->GetZSlice());

  vtksys_stl::string viewer_tclname(
    vtkKWTkUtilities::GetTclNameFromPointer(app, viewer));

  char command[1024];
  sprintf(command, "%s SetZSlice [%s GetValue] ; %s Render", 
          viewer_tclname.c_str(), slice_scale->GetTclName(), rw->GetTclName());
  cout << command << endl;
  slice_scale->SetCommand(NULL, command);

  app->Script("pack %s -side top -expand n -fill x -padx 2 -pady 2", 
              slice_scale->GetWidgetName());

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
  slice_scale->Delete();
  viewer->Delete();
  iren->Delete();
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
