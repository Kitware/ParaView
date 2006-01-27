#include "vtkActor.h"
#include "vtkKWApplication.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWSurfaceMaterialPropertyWidget.h"
#include "vtkKWWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkKWSimpleAnimationWidget.h"

#include <itkCylinderSpatialObject.h>
#include <sovVTKRenderer3D.h>

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
  app->SetName("KWSOViewerExample");
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

  // Create a SOV renderer, and use it in the renderwidget

  sov::VTKRenderer3D::Pointer sov_renderer = sov::VTKRenderer3D::New();
  rw->RemoveAllRenderers();
  rw->AddRenderer(sov_renderer->GetVTKRenderer());

  // Create a cylinder spatial object

  itk::CylinderSpatialObject::Pointer cylinder = 
    itk::CylinderSpatialObject::New();
  cylinder->GetProperty()->SetColor(0.0,0.0,1.0);

  // Create a scene spatial object, add the cylinder and render it

  itk::SceneSpatialObject<3>::Pointer scene = 
    itk::SceneSpatialObject<3>::New();
  scene->AddSpatialObject(cylinder);
  sov_renderer->SetScene(scene);
  sov_renderer->Update();

  // Create a material property editor

  vtkKWSurfaceMaterialPropertyWidget *mat_prop_widget = 
    vtkKWSurfaceMaterialPropertyWidget::New();
  mat_prop_widget->SetParent(win->GetMainPanelFrame());
  mat_prop_widget->Create();
  mat_prop_widget->SetPropertyChangedCommand(rw, "Render");
  mat_prop_widget->SetPropertyChangingCommand(rw, "Render");

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              mat_prop_widget->GetWidgetName());

  // Get the first actor in the scene, and control its material property

  vtkActorCollection* actors = sov_renderer->GetVTKRenderer()->GetActors();
  actors->InitTraversal();
  vtkActor *actor = actors->GetNextActor();
  if (actor)
    {
    mat_prop_widget->SetProperty(actor->GetProperty());
    }

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
