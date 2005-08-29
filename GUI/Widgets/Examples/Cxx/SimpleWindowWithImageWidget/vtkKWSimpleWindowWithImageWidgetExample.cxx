#include "vtkKWSimpleWindowWithImageWidgetExample.h"

#include "vtkObjectFactory.h"
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
#include "vtkKWWindowLevelPresetSelector.h"

#include "vtkKWWidgetsConfigurePaths.h"
#include "vtkToolkits.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSimpleWindowWithImageWidgetExample );
vtkCxxRevisionMacro(vtkKWSimpleWindowWithImageWidgetExample, "1.1");

//----------------------------------------------------------------------------
int vtkKWSimpleWindowWithImageWidgetExample::Run(int argc, char *argv[])
{
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

  vtkKWApplication *app = this->GetApplication();
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
  
  this->RenderWidget = vtkKWRenderWidget::New();
  this->RenderWidget->SetParent(win->GetViewFrame());
  this->RenderWidget->Create(app);
  this->RenderWidget->CornerAnnotationVisibilityOn();

  app->Script("pack %s -expand y -fill both -anchor c -expand y", 
              this->RenderWidget->GetWidgetName());

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
  this->SliceScale->Create(app);
  this->SliceScale->SetCommand(this, "SetSliceCallback");

  this->UpdateSliceScale();

  app->Script("pack %s -side top -expand n -fill x -padx 2 -pady 2", 
              this->SliceScale->GetWidgetName());

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

  vtkKWMenuButton *mb = orientation_menubutton->GetWidget()->GetWidget();
  mb->AddRadioButton("X-Y", this, "SetSliceOrientationToXYCallback");
  mb->AddRadioButton("X-Z", this, "SetSliceOrientationToXZCallback");
  mb->AddRadioButton("Y-Z", this, "SetSliceOrientationToYZCallback");
  mb->SetValue("X-Y");

  // Create a window/level preset selector

  this->WindowLevelPresetSelector = vtkKWWindowLevelPresetSelector::New();

  this->WindowLevelPresetSelector->SetParent(win->GetMainPanelFrame());
  this->WindowLevelPresetSelector->Create(app);
  this->WindowLevelPresetSelector->SetPadX(2);
  this->WindowLevelPresetSelector->SetPadY(2);
  this->WindowLevelPresetSelector->SetBorderWidth(2);
  this->WindowLevelPresetSelector->SetReliefToGroove();
  this->WindowLevelPresetSelector->SetAddWindowLevelPresetCommand(
    this, "AddWindowLevelPresetCallback");
  this->WindowLevelPresetSelector->SetApplyWindowLevelPresetCommand(
    this, "ApplyWindowLevelPresetCallback");
  
  app->Script("pack %s -side top -anchor nw -expand n -fill x -pady 2",
              this->WindowLevelPresetSelector->GetWidgetName());
  
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
  this->SliceScale->Delete();
  orientation_menubutton->Delete();
  this->ImageViewer->Delete();
  this->RenderWidget->Delete();
  this->WindowLevelPresetSelector->Delete();
  win->Delete();
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::SetSliceCallback()
{
  this->ImageViewer->SetSlice(this->SliceScale->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::UpdateSliceScale()
{
  this->SliceScale->SetRange(
    this->ImageViewer->GetSliceMin(), this->ImageViewer->GetSliceMax());
  this->SliceScale->SetValue(this->ImageViewer->GetSlice());
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::SetSliceOrientationToXYCallback()
{
  this->ImageViewer->SetSliceOrientationToXY();
  this->UpdateSliceScale();
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::SetSliceOrientationToXZCallback()
{
  this->ImageViewer->SetSliceOrientationToXZ();
  this->UpdateSliceScale();
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::SetSliceOrientationToYZCallback()
{
  this->ImageViewer->SetSliceOrientationToYZ();
  this->UpdateSliceScale();
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::AddWindowLevelPresetCallback()
{
  int id = this->WindowLevelPresetSelector->AddWindowLevelPreset(
    this->ImageViewer->GetColorWindow(), this->ImageViewer->GetColorLevel());
  this->WindowLevelPresetSelector->SetWindowLevelPresetImageFromRenderWindow(
    id, this->RenderWidget->GetRenderWindow());
}

//----------------------------------------------------------------------------
void vtkKWSimpleWindowWithImageWidgetExample::ApplyWindowLevelPresetCallback(
  int id)
{
  double *wl = this->WindowLevelPresetSelector->GetWindowLevelPreset(id);
  if (wl)
    {
    this->ImageViewer->SetColorWindow(wl[0]);
    this->ImageViewer->SetColorLevel(wl[1]);
    this->ImageViewer->Render();
    }
}
