#include "vtkKWMedicalImageViewerExample.h"

#include "vtkCornerAnnotation.h"
#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithSpinButtons.h"
#include "vtkKWMenuButtonWithSpinButtonsWithLabel.h"
#include "vtkKWNotebook.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWScale.h"
#include "vtkKWSimpleAnimationWidget.h"
#include "vtkKWWindow.h"
#include "vtkKWWindowLevelPresetSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"

#include "vtkKWWidgetsPaths.h"
#include "vtkToolkits.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMedicalImageViewerExample );
vtkCxxRevisionMacro(vtkKWMedicalImageViewerExample, "1.3");

//----------------------------------------------------------------------------
int vtkKWMedicalImageViewerExample::Run(int argc, char *argv[])
{
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

  vtkKWApplication *app = this->GetApplication();
  app->SetName("KWMedicalImageViewerExample");
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
  
  this->RenderWidget = vtkKWRenderWidget::New();
  this->RenderWidget->SetParent(win->GetViewFrame());
  this->RenderWidget->Create();
  this->RenderWidget->CornerAnnotationVisibilityOn();

  app->Script("pack %s -expand y -fill both -anchor c -expand y", 
              this->RenderWidget->GetWidgetName());

  // Create a volume reader

  vtkXMLImageDataReader *reader = vtkXMLImageDataReader::New();

  char data_path[2048];
  sprintf(data_path, "%s/Data/head100x100x47.vti", KWWidgets_EXAMPLES_DIR);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/..%s/Examples/Data/head100x100x47.vti",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR);
    }
  reader->SetFileName(data_path);

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  this->ImageViewer = vtkImageViewer2::New();
  this->ImageViewer->SetRenderWindow(this->RenderWidget->GetRenderWindow());
  this->ImageViewer->SetRenderer(this->RenderWidget->GetRenderer());
  this->ImageViewer->SetInput(reader->GetOutput());
  this->ImageViewer->SetupInteractor(
    this->RenderWidget->GetRenderWindow()->GetInteractor());

  // Reset the window/level and the camera

  reader->Update();
  double *range = reader->GetOutput()->GetScalarRange();
  this->ImageViewer->SetColorWindow(range[1] - range[0]);
  this->ImageViewer->SetColorLevel(0.5 * (range[1] + range[0]));

  this->RenderWidget->ResetCamera();

  // The corner annotation has the ability to parse "tags" and fill
  // them with information gathered from other objects.
  // For example, let's display the slice and window/level in one corner
  // by connecting the corner annotation to our image actor and
  // image mapper

  vtkCornerAnnotation *ca = this->RenderWidget->GetCornerAnnotation();
  ca->SetImageActor(this->ImageViewer->GetImageActor());
  ca->SetWindowLevel(this->ImageViewer->GetWindowLevel());
  ca->SetText(2, "<slice>");
  ca->SetText(3, "<window>\n<level>");

  // Create a scale to control the slice

  this->SliceScale = vtkKWScale::New();
  this->SliceScale->SetParent(win->GetViewPanelFrame());
  this->SliceScale->Create();
  this->SliceScale->SetCommand(this, "SetSliceFromScaleCallback");

  app->Script("pack %s -side top -expand n -fill x -padx 2 -pady 2", 
              this->SliceScale->GetWidgetName());

  // Create a menu button to control the orientation

  vtkKWMenuButtonWithSpinButtonsWithLabel *orientation_menubutton = 
    vtkKWMenuButtonWithSpinButtonsWithLabel::New();

  orientation_menubutton->SetParent(win->GetMainPanelFrame());
  orientation_menubutton->Create();
  orientation_menubutton->SetLabelText("Orientation:");
  orientation_menubutton->SetPadX(2);
  orientation_menubutton->SetPadY(2);
  orientation_menubutton->SetBorderWidth(2);
  orientation_menubutton->SetReliefToGroove();

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              orientation_menubutton->GetWidgetName());

  vtkKWMenuButton *mb = orientation_menubutton->GetWidget()->GetWidget();
  mb->AddRadioButton("X-Y", this, "SetSliceOrientationToXYCallback");
  mb->AddRadioButton("X-Z", this, "SetSliceOrientationToXZCallback");
  mb->AddRadioButton("Y-Z", this, "SetSliceOrientationToYZCallback");
  mb->SetValue("X-Y");

  // Create a window/level preset selector

  vtkKWFrameWithLabel *wl_frame = vtkKWFrameWithLabel::New();
  wl_frame->SetParent(win->GetMainPanelFrame());
  wl_frame->Create();
  wl_frame->SetLabelText("Window/Level Presets");

  app->Script("pack %s -side top -anchor nw -expand n -fill x -pady 2",
              wl_frame->GetWidgetName());

  this->WindowLevelPresetSelector = vtkKWWindowLevelPresetSelector::New();

  this->WindowLevelPresetSelector->SetParent(wl_frame->GetFrame());
  this->WindowLevelPresetSelector->Create();
  this->WindowLevelPresetSelector->ThumbnailColumnVisibilityOn();
  this->WindowLevelPresetSelector->SetPresetAddCommand(
    this, "WindowLevelPresetAddCallback");
  this->WindowLevelPresetSelector->SetPresetApplyCommand(
    this, "WindowLevelPresetApplyCallback");
  this->WindowLevelPresetSelector->SetPresetUpdateCommand(
    this, "WindowLevelPresetUpdateCallback");
  this->WindowLevelPresetSelector->SetPresetHasChangedCommand(
    this, "WindowLevelPresetHasChangedCallback");
  
  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              this->WindowLevelPresetSelector->GetWidgetName());

  // Create a simple animation widget

  vtkKWFrameWithLabel *animation_frame = vtkKWFrameWithLabel::New();
  animation_frame->SetParent(win->GetMainPanelFrame());
  animation_frame->Create();
  animation_frame->SetLabelText("Movie Creator");

  app->Script("pack %s -side top -anchor nw -expand n -fill x -pady 2",
              animation_frame->GetWidgetName());
  this->AnimationWidget = vtkKWSimpleAnimationWidget::New();
  this->AnimationWidget->SetParent(animation_frame->GetFrame());
  this->AnimationWidget->Create();
  this->AnimationWidget->SetRenderWidget(this->RenderWidget);
  this->AnimationWidget->SetAnimationTypeToSlice();
  this->AnimationWidget->SetSliceSetCommand(this, "SetSliceCallback");
  this->AnimationWidget->SetSliceGetCommand(this, "GetSliceCallback");
  this->AnimationWidget->SetSliceGetMinAndMaxCommands(
    this, "GetSliceMinCallback", "GetSliceMaxCallback");
  this->AnimationWidget->ProvideEnoughFramesForSlicesOn();

  app->Script("pack %s -side top -anchor nw -expand n -fill x",
              this->AnimationWidget->GetWidgetName());

  this->UpdateSliceRanges();

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

  reader->Delete();
  this->SliceScale->Delete();
  orientation_menubutton->Delete();
  this->ImageViewer->Delete();
  this->RenderWidget->Delete();
  wl_frame->Delete();
  this->WindowLevelPresetSelector->Delete();
  animation_frame->Delete();
  this->AnimationWidget->Delete();
  win->Delete();

  return ret;
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::SetSliceFromScaleCallback(
  double value)
{
  this->ImageViewer->SetSlice((int)value);
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::SetSliceCallback(int slice)
{
  this->ImageViewer->SetSlice(slice);
}

//----------------------------------------------------------------------------
int vtkKWMedicalImageViewerExample::GetSliceCallback()
{
  return this->ImageViewer->GetSlice();
}

//----------------------------------------------------------------------------
int vtkKWMedicalImageViewerExample::GetSliceMinCallback()
{
  return this->ImageViewer->GetSliceMin();
}

//----------------------------------------------------------------------------
int vtkKWMedicalImageViewerExample::GetSliceMaxCallback()
{
  return this->ImageViewer->GetSliceMax();
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::UpdateSliceRanges()
{
  this->SliceScale->SetRange(
    this->ImageViewer->GetSliceMin(), this->ImageViewer->GetSliceMax());
  this->SliceScale->SetValue(this->ImageViewer->GetSlice());

  // This will update the starting slice and ending slice sliders

  this->AnimationWidget->Update();
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::SetSliceOrientationToXYCallback()
{
  this->ImageViewer->SetSliceOrientationToXY();
  this->UpdateSliceRanges();
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::SetSliceOrientationToXZCallback()
{
  this->ImageViewer->SetSliceOrientationToXZ();
  this->UpdateSliceRanges();
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::SetSliceOrientationToYZCallback()
{
  this->ImageViewer->SetSliceOrientationToYZ();
  this->UpdateSliceRanges();
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::WindowLevelPresetApplyCallback(
  int id)
{
  if (this->WindowLevelPresetSelector->HasPreset(id))
    {
    this->ImageViewer->SetColorWindow(
      this->WindowLevelPresetSelector->GetPresetWindow(id));
    this->ImageViewer->SetColorLevel(
      this->WindowLevelPresetSelector->GetPresetLevel(id));
    this->ImageViewer->Render();
    }
}
//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::WindowLevelPresetAddCallback()
{
  this->WindowLevelPresetUpdateCallback(
    this->WindowLevelPresetSelector->AddPreset());
}

//----------------------------------------------------------------------------
void vtkKWMedicalImageViewerExample::WindowLevelPresetUpdateCallback(
  int id)
{
  this->WindowLevelPresetSelector->SetPresetWindow(
    id, this->ImageViewer->GetColorWindow());
  this->WindowLevelPresetSelector->SetPresetLevel(
    id, this->ImageViewer->GetColorLevel());
  this->WindowLevelPresetHasChangedCallback(id);
}

//----------------------------------------------------------------------------
void 
vtkKWMedicalImageViewerExample::WindowLevelPresetHasChangedCallback(
  int id)
{
  this->WindowLevelPresetSelector->
    BuildPresetThumbnailAndScreenshotFromRenderWindow(
      id, this->RenderWidget->GetRenderWindow());
}
