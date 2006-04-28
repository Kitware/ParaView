#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWMyBlueTheme.h"
#include "vtkKWMyGreenTheme.h"
#include "vtkKWRadioButton.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkKWText.h"

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
  app->SetName("KWThemeExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    }
  app->RestoreApplicationSettingsFromRegistry();

  // Create a few themes

  vtkKWMyBlueTheme *blue_theme = vtkKWMyBlueTheme::New();
  vtkKWMyGreenTheme *green_theme = vtkKWMyGreenTheme::New();

  // Let's create a simple message dialog.

  app->PromptBeforeExitOff();

  int theme = 0;
  int keep_going = !option_test;
  while (keep_going || !app->Exit())
    {
    // Select the theme

    if (theme == 0)
      {
      app->SetTheme(NULL);
      }
    else if (theme == 1)
      {
      app->SetTheme(blue_theme);
      } 
    else if (theme == 2)
      {
      app->SetTheme(green_theme);
      }

    // Create the message dialog

    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->SetApplication(app);
    dlg->SetStyleToCancel();
    dlg->Create();
    dlg->SetTitle(app->GetName());
    dlg->SetText("This example demonstrates simple themes using the vtkKWTheme class. Select a theme below and this dialog will be re-created using the corresponding settings.");

    // The themes selection

    vtkKWFrameWithLabel *themes_frame = vtkKWFrameWithLabel::New();
    themes_frame->SetParent(dlg->GetBottomFrame());
    themes_frame->Create();
    themes_frame->SetLabelText("Themes");
    themes_frame->SetWidth(300);

    app->Script("pack %s -side top -pady 2 -padx 2", 
                themes_frame->GetWidgetName());

    vtkKWRadioButtonSet *themes_set = vtkKWRadioButtonSet::New();
    themes_set->SetParent(themes_frame->GetFrame());
    themes_set->Create();
    
    app->Script("pack %s -side top -anchor w", themes_set->GetWidgetName());

    vtkKWRadioButton *default_theme_rb = themes_set->AddWidget(0);
    default_theme_rb->SetText("Default Theme");
    default_theme_rb->SetCommand(dlg, "OK");

    vtkKWRadioButton *blue_theme_rb = themes_set->AddWidget(1);
    blue_theme_rb->SetText("Blue Theme");
    blue_theme_rb->SetCommand(dlg, "OK");

    vtkKWRadioButton *green_theme_rb = themes_set->AddWidget(2);
    green_theme_rb->SetText("Green Theme");
    green_theme_rb->SetCommand(dlg, "OK");

    green_theme_rb->SetVariableValueAsInt(theme);

    // Invoke the dialog, and switch the theme

    dlg->Invoke();

    theme = green_theme_rb->GetVariableValueAsInt();
    keep_going = dlg->GetStatus() != vtkKWMessageDialog::StatusCanceled;

    themes_frame->Delete();
    themes_set->Delete();
    dlg->Delete();
    }

  // If --test was provided, do not prompt the message dialog and run this
  // example as a non-interactive test for software quality purposes.

  int ret = 0;
  if (option_test)
    {
    
    }

  // Deallocate and exit
  
  blue_theme->Delete();
  green_theme->Delete();
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
