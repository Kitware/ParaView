#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkKWNotebook.h"
#include "vtkKWListBox.h"
#include "vtkKWUserInterfaceManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWApplication.h"

#include "KWWidgetsTourExample.h"

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

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kitware.com");

  // Add a window to the application
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindow *win = vtkKWWindow::New();
  win->SupportHelpOn();
  app->AddWindow(win);
  win->Create(app, NULL);

  // Add a user interface panel to the main user interface manager

  vtkKWUserInterfacePanel *widgets_panel = vtkKWUserInterfacePanel::New();
  widgets_panel->SetName("Widgets Interface");
  widgets_panel->SetUserInterfaceManager(
    win->GetMainUserInterfaceManager());
  widgets_panel->Create(app);

  widgets_panel->AddPage("Widgets", "Select a widget", NULL);
  vtkKWWidget *page_widget = widgets_panel->GetPageWidget("Widgets");

  // Add a list box to pick a widget example

  vtkKWListBox *widgets_list = vtkKWListBox::New();
  widgets_list->SetParent(page_widget);
  widgets_list->ScrollbarOn();
  widgets_list->Create(app, NULL);
  widgets_list->SetHeight(300);

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              widgets_list->GetWidgetName());

  widgets_panel->Raise();

  // Add a user interface panel to the secondary user interface manager

  vtkKWUserInterfacePanel *source_panel = vtkKWUserInterfacePanel::New();
  source_panel->SetName("Source Interface");
  source_panel->SetUserInterfaceManager(
    win->GetSecondaryUserInterfaceManager());
  source_panel->Create(app);
  win->GetSecondaryNotebook()->AlwaysShowTabsOff();

  win->GetViewNotebook()->ShowOnlyPagesWithSameTagOn();

  // Add text widget to display the Tcl example source

  source_panel->AddPage("Tcl Source", "Display the Tcl example source", NULL);
  page_widget = source_panel->GetPageWidget("Tcl Source");

  vtkKWText *tcl_source_text = vtkKWText::New();
  tcl_source_text->SetParent(page_widget);
  tcl_source_text->EditableTextOff();
  tcl_source_text->UseVerticalScrollbarOn();
  tcl_source_text->Create(app, NULL);
  tcl_source_text->SetWrapToNone();

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              tcl_source_text->GetWidgetName());

  // Add text widget to display the C++ example source

  source_panel->AddPage("C++ Source", "Display the C++ example source", NULL);
  page_widget = source_panel->GetPageWidget("C++ Source");

  vtkKWText *cxx_source_text = vtkKWText::New();
  cxx_source_text->SetParent(page_widget);
  cxx_source_text->EditableTextOff();
  cxx_source_text->UseVerticalScrollbarOn();
  cxx_source_text->Create(app, NULL);
  cxx_source_text->SetWrapToNone();

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              cxx_source_text->GetWidgetName());

  source_panel->RaisePage("C++ Source");

  // Populate the examples
  // Create a panel for each one, and pass the frame

  WidgetNode *node_ptr = Widgets;
  while (node_ptr->Name && node_ptr->EntryPoint)
    {
    vtkKWUserInterfacePanel *panel = vtkKWUserInterfacePanel::New();
    panel->SetName(node_ptr->Name);
    panel->SetUserInterfaceManager(win->GetViewUserInterfaceManager());
    panel->Create(app);
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL, NULL);

    vtkKWFrameWithScrollbar *framews = vtkKWFrameWithScrollbar::New();
    framews->SetParent(panel->GetPageWidget(panel->GetName()));
    framews->Create(app, NULL);
    framews->Delete();

    app->Script("pack %s -side top -expand y -fill both", 
                framews->GetWidgetName());

    if ((*node_ptr->EntryPoint)(framews->GetFrame()))
      {
      widgets_list->AppendUnique(node_ptr->Name);

      // Try to find the C++ source

      vtksys_stl::string example_source(node_ptr->Name);
      example_source += ".cxx";

      vtksys_stl::string example_source_path(SourceDir);
      example_source_path += '/';
      example_source_path += WidgetsDirName;
      example_source_path += '/';
      example_source_path += example_source;

      if (!vtksys::SystemTools::FileExists(example_source_path.c_str()))
        {
        example_source_path = app->GetInstallationDirectory();
        example_source_path += "/../share/";
        example_source_path += ProjectName;
        example_source_path += "/Examples/Cxx/";
        example_source_path += ExampleDirName;
        example_source_path += '/';
        example_source_path += WidgetsDirName;
        example_source_path += '/';
        example_source_path += example_source;
        }
      if (vtksys::SystemTools::FileExists(example_source_path.c_str()))
        {
        app->Script("set cxx_source(%s) [read [open %s]]", 
                    node_ptr->Name, example_source_path.c_str());
        }
      else
        {
        app->Script("set cxx_source(%s) {}", node_ptr->Name);
        }

      // Try to find the Tcl source

      example_source = node_ptr->Name;
      example_source += ".tcl";

      example_source_path = SourceDir;
      example_source_path += "/../../Tcl/";
      example_source_path += ExampleDirName;
      example_source_path += '/';
      example_source_path += WidgetsDirName;
      example_source_path += '/';
      example_source_path += example_source;

      if (!vtksys::SystemTools::FileExists(example_source_path.c_str()))
        {
        example_source_path = app->GetInstallationDirectory();
        example_source_path += "/../share/";
        example_source_path += ProjectName;
        example_source_path += "/Examples/Tcl/";
        example_source_path += ExampleDirName;
        example_source_path += '/';
        example_source_path += WidgetsDirName;
        example_source_path += '/';
        example_source_path += example_source;
        }
      if (vtksys::SystemTools::FileExists(example_source_path.c_str()))
        {
        app->Script("set tcl_source(%s) [read [open %s]]", 
                    node_ptr->Name, example_source_path.c_str());
        }
      else
        {
        app->Script("set tcl_source(%s) {}", node_ptr->Name);
        }
      }
    node_ptr++;
    }

  vtksys_stl::string cmd;

  // Raise the example panel

  cmd += win->GetTclName();
  cmd += " ShowViewUserInterface [";
  cmd += widgets_list->GetTclName();
  cmd += " GetSelection]";

  // Show the C++ example source

  cmd += ";";
  cmd += cxx_source_text->GetTclName();
  cmd += " SetValue $cxx_source([";
  cmd += widgets_list->GetTclName();
  cmd += " GetSelection])";

  // Show the Tcl example source

  cmd += ";";
  cmd += tcl_source_text->GetTclName();
  cmd += " SetValue $tcl_source([";
  cmd += widgets_list->GetTclName();
  cmd += " GetSelection])";

  widgets_list->SetSingleClickCallback(NULL, cmd.c_str());
  
  // Start the application

  app->Start(argc, argv);
  int ret = app->GetExitStatus();

  // Deallocate and exit

  win->Delete();
  widgets_panel->Delete();
  widgets_list->Delete();
  source_panel->Delete();
  tcl_source_text->Delete();
  cxx_source_text->Delete();
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
