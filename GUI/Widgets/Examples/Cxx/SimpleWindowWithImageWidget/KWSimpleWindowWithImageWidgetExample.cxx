#include "vtkCornerAnnotation.h"
#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithSpinButtons.h"
#include "vtkKWMenuButtonWithSpinButtonsWithLabel.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWScale.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWNotebook.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"

#include "vtkKWWidgetsConfigurePaths.h"
#include "vtkToolkits.h"

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

  app->SetHelpDialogStartingPage("http://public.kitware.com/KWWidgets");

  // Add a window
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindow *win = vtkKWWindow::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create(app);
  win->SecondaryPanelVisibilityOff();

  // Add a render widget, attach it to the view frame, and pack
  
  vtkKWRenderWidget *rw = vtkKWRenderWidget::New();

  rw->SetParent(win->GetViewFrame());
  rw->Create(app);
  rw->CornerAnnotationVisibilityOn();

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
  viewer->SetupInteractor(rw->GetRenderWindow()->GetInteractor());

  // Reset the window/level and the camera

  reader->Update();
  double *range = reader->GetOutput()->GetScalarRange();
  viewer->SetColorWindow(range[1] - range[0]);
  viewer->SetColorLevel(0.5 * (range[1] + range[0]));

  rw->ResetCamera();

  // The corner annotation has the ability to parse "tags" and fill
  // them with information gathered from other objects.
  // For example, let's display the slice and window/level in one corner
  // by connecting the corner annotation to our image actor and
  // image mapper

  vtkCornerAnnotation *ca = rw->GetCornerAnnotation();
  ca->SetImageActor(viewer->GetImageActor());
  ca->SetWindowLevel(viewer->GetWindowLevel());
  ca->SetText(2, "<slice>");
  ca->SetText(3, "<window>\n<level>");

  // The above code makes sense only if we can get the Tcl object 
  // corresponding to the viewer. I.e., VTK_WRAP_TCL must be ON

#ifdef VTK_WRAP_TCL

  vtksys_stl::string viewer_tclname(
    vtkKWTkUtilities::GetTclNameFromPointer(app, viewer));

  // Create a scale to control the slice

  vtkKWScale *slice_scale = vtkKWScale::New();
  slice_scale->SetParent(win->GetViewPanelFrame());
  slice_scale->Create(app);
  slice_scale->SetRange(viewer->GetSliceMin(), viewer->GetSliceMax());
  slice_scale->SetValue(viewer->GetSlice());

  char command[1024];
  sprintf(command, "%s SetSlice [%s GetValue]", 
          viewer_tclname.c_str(), slice_scale->GetTclName(), rw->GetTclName());
  slice_scale->SetCommand(NULL, command);

  app->Script("pack %s -side top -expand n -fill x -padx 2 -pady 2", 
              slice_scale->GetWidgetName());

  // Create a menu button to control the orientation

  vtkKWMenuButtonWithSpinButtonsWithLabel *orientation_menubutton = 
    vtkKWMenuButtonWithSpinButtonsWithLabel::New();

  orientation_menubutton->SetParent(win->GetMainPanelFrame());
  orientation_menubutton->Create(app);
  orientation_menubutton->SetLabelText("Orientation:");
  orientation_menubutton->SetPadX(2);
  orientation_menubutton->SetPadY(2);
  orientation_menubutton->SetBorderWidth(2);
  orientation_menubutton->SetReliefToGroove();

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              orientation_menubutton->GetWidgetName());

  // This is ugly, this is *not* the way it should be done in your application,
  // you should have your class Tcl wrapped automatically and call
  // the corresponding callbacks instead of creating Tk procs

  app->Script("proc update_scale {} { %s SetRange [%s GetSliceMin] [%s GetSliceMax] ; %s SetValue [%s GetSlice] }",
              slice_scale->GetTclName(), 
              viewer_tclname.c_str(), 
              viewer_tclname.c_str(), 
              slice_scale->GetTclName(), 
              viewer_tclname.c_str());
              
  vtkKWMenuButton *mb = orientation_menubutton->GetWidget()->GetWidget();
  mb->AddRadioButton("X-Y", viewer, "SetSliceOrientationToXY ; update_scale");
  mb->AddRadioButton("X-Z", viewer, "SetSliceOrientationToXZ ; update_scale");
  mb->AddRadioButton("Y-Z", viewer, "SetSliceOrientationToYZ ; update_scale");
  mb->SetValue("X-Y");

#endif

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
#ifdef VTK_WRAP_TCL
  slice_scale->Delete();
  orientation_menubutton->Delete();
#endif
  viewer->Delete();
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
