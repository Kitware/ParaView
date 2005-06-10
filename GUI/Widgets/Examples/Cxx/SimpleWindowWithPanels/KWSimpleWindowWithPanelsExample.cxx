#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWHSVColorSelector.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWindow.h"
#include "vtkMath.h"

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

  int i;
  char buffer[1024];

  // Create the application
  // Restore the settings that have been saved to the registry, like
  // the geometry of the user interface so far.

  vtkKWApplication *app = vtkKWApplication::New();
  app->RestoreApplicationSettingsFromRegistry();
  app->SetName("KWSimpleWindowWithPanelsExample");

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kitware.com");

  // Add a window to the application
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindow *win = vtkKWWindow::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create(app, NULL);

  win->GetViewFrame()->SetBackgroundColor(0.92, 0.87, 0.69);

  // Add a label in the center, attach it to the view frame, and pack
  
  vtkKWLabel *hello_label = vtkKWLabel::New();
  hello_label->SetParent(win->GetViewFrame());
  hello_label->Create(app, NULL);
  hello_label->SetWidth(50);
  hello_label->SetForegroundColor(1.0, 1.0, 1.0);
  hello_label->SetBackgroundColor(0.2, 0.2, 0.4);

  app->Script(
    "pack %s -side left -anchor c -expand y -ipadx 60 -ipady 60", 
    hello_label->GetWidgetName());

  // Add a user interface panel to the secondary user interface manager
  // Note how we do not interfere with the notebook, we just create
  // a panel, hand it over to the manager, and ask the panel to return
  // us where we should pack our widgets (i.e., the actually implementation
  // is left to the window). 

  vtkKWUserInterfacePanel *label_panel = vtkKWUserInterfacePanel::New();
  label_panel->SetName("Label Interface");
  label_panel->SetUserInterfaceManager(
    win->GetSecondaryUserInterfaceManager());
  label_panel->Create(app);

  label_panel->AddPage("Language", "Change the label language", NULL);
  vtkKWWidget *page_widget = label_panel->GetPageWidget("Language");

  // Add a set of radiobutton to modify the label text
  // The set makes sure each radiobutton share the same 'variable name', 
  // i.e. only one can be selected in the set because they reference
  // the same Tcl variable internally and each radiobutton sets it to a
  // different value (arbitrarily set to the ID of the button by default)

  vtkKWRadioButtonSet *rbs = vtkKWRadioButtonSet::New();
  rbs->SetParent(page_widget);
  rbs->Create(app, NULL);

  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              rbs->GetWidgetName());

  const char* texts[] = { "Hello, World", "Bonjour, Monde", "Hallo, Welt"};
  vtkKWRadioButton *rb = NULL;
  for (size_t id = 0; id < sizeof(texts) / sizeof(texts[0]); id++)
    {
    rb = rbs->AddWidget(id);
    rb->SetText(texts[id]);
    sprintf(buffer, "SetText {%s}", texts[id]);
    rb->SetCommand(hello_label, buffer);
    rb->SetBalloonHelpString("Set label text");
    }

  // Select the first label text in the set, and show/raise  the panel now
  // that we have added our UI

  hello_label->SetText(texts[0]);
  rbs->GetWidget(0)->StateOn(); // or rb->SetVariableValue(0);

  label_panel->Raise();

  // Add a user interface panel to the main user interface manager

  vtkKWUserInterfacePanel *frame_panel = vtkKWUserInterfacePanel::New();
  frame_panel->SetName("View Interface");
  frame_panel->SetUserInterfaceManager(
    win->GetMainUserInterfaceManager());
  frame_panel->Create(app);

  frame_panel->AddPage("View Colors", "Change the view colors", NULL);
  page_widget = frame_panel->GetPageWidget("View Colors");

  // Add a HSV color selector to set the view frame color
  // Put it inside a labeled frame for kicks

  vtkKWFrameLabeled *ccb_frame = vtkKWFrameLabeled::New();
  ccb_frame->SetParent(page_widget);
  ccb_frame->Create(app, NULL);
  ccb_frame->SetLabelText("View Background Color");

  app->Script(
    "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2", 
    ccb_frame->GetWidgetName());

  vtkKWHSVColorSelector *ccb = vtkKWHSVColorSelector::New();
  ccb->SetParent(ccb_frame->GetFrame());
  ccb->Create(app, NULL);
  ccb->SetSelectionChangingCommand(
    hello_label->GetParent(), "SetBackgroundColor");
  ccb->InvokeCommandsWithRGBOn();
  ccb->SetBalloonHelpString("Set the view background color");

  ccb->SetSelectedColor(
    vtkMath::RGBToHSV(hello_label->GetParent()->GetBackgroundColor()));

  app->Script("pack %s -side top -anchor w -expand y -padx 2 -pady 2", 
              ccb->GetWidgetName());

  frame_panel->Raise();

  // Create a main toolbar to control the label foreground color
  // Set its name and it will be automatically added to the toolbars menu,
  // and its visibility will be saved to the registry.

  vtkKWToolbar *fg_toolbar = vtkKWToolbar::New();
  fg_toolbar->SetName("Label Foreground Color");
  fg_toolbar->SetParent(win->GetMainToolbarSet()->GetToolbarsFrame());
  fg_toolbar->Create(app);
  win->GetMainToolbarSet()->AddToolbar(fg_toolbar);

  // Add a simple explanatory label at the beginning of the toolbar 
  // (most of the time, you should not really need that, icons should be
  // self explanatory).

  vtkKWLabel *fg_toolbar_label = vtkKWLabel::New();
  fg_toolbar_label->SetParent(fg_toolbar->GetFrame());
  fg_toolbar_label->Create(app, NULL);
  fg_toolbar_label->SetText("Label Foreground:");
  fg_toolbar->AddWidget(fg_toolbar_label);
  fg_toolbar_label->Delete();

  // Create a secondary toolbar to control the label background color
  // Set its name and it will be automatically added to the toolbars menu,
  // and its visibility will be saved to the registry.

  vtkKWToolbar *bg_toolbar = vtkKWToolbar::New();
  bg_toolbar->SetName("Label Background Color");
  bg_toolbar->SetParent(win->GetSecondaryToolbarSet()->GetToolbarsFrame());
  bg_toolbar->Create(app);
  win->GetSecondaryToolbarSet()->AddToolbar(bg_toolbar);

  // Add a simple explanatory label at the beginning of the toolbar 
  // (most of the time, you should not really need that, icons should be
  // self explanatory).

  vtkKWLabel *bg_toolbar_label = vtkKWLabel::New();
  bg_toolbar_label->SetParent(bg_toolbar->GetFrame());
  bg_toolbar_label->Create(app, NULL);
  bg_toolbar_label->SetText("Label Background:");
  bg_toolbar->AddWidget(bg_toolbar_label);
  bg_toolbar_label->Delete();

  // Add some color button to the toolbars
  // Each button will set the label foreground or (a darker) background color
  
  const int nb_buttons = 10;
  for (i = 0; i < nb_buttons; i++)
    {
    double hsv[3];
    hsv[0] = (double)i * (1.0 / (double)nb_buttons);
    double *rgb;

    hsv[1] = hsv[2] = 1.0;
    rgb = vtkMath::HSVToRGB(hsv);
    vtkKWPushButton *fg_button = vtkKWPushButton::New();
    fg_button->SetParent(fg_toolbar->GetFrame());
    fg_button->Create(app, NULL);
    sprintf(buffer, "SetForegroundColor %lf %lf %lf", rgb[0], rgb[1], rgb[2]);
    fg_button->SetCommand(hello_label, buffer);
    fg_button->SetWidth(2);
    fg_button->SetBackgroundColor(rgb);
    fg_button->SetBalloonHelpString("Set the label foreground color");
    fg_toolbar->AddWidget(fg_button);
    fg_button->Delete();

    hsv[1] = hsv[2] = 0.5;
    rgb = vtkMath::HSVToRGB(hsv);
    vtkKWPushButton *bg_button = vtkKWPushButton::New();
    bg_button->SetParent(bg_toolbar->GetFrame());
    bg_button->Create(app, NULL);
    sprintf(buffer, "SetBackgroundColor %lf %lf %lf", rgb[0], rgb[1], rgb[2]);
    bg_button->SetCommand(hello_label, buffer);
    bg_button->SetWidth(2);
    bg_button->SetBackgroundColor(rgb);
    bg_button->SetBalloonHelpString("Set the label background color");
    bg_toolbar->AddWidget(bg_button);
    bg_button->Delete();
    }
  
  // Start the application

  app->Start(argc, argv);
  int ret = app->GetExitStatus();

  // Deallocate and exit

  ccb->Delete();
  fg_toolbar->Delete();
  bg_toolbar->Delete();
  ccb_frame->Delete();
  label_panel->Delete();
  frame_panel->Delete();
  hello_label->Delete();
  rbs->Delete();
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
