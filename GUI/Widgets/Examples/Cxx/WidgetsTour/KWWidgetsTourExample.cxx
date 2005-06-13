#include "vtkKWApplication.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWListBox.h"
#include "vtkKWNotebook.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWText.h"
#include "vtkKWTextLabeled.h"
#include "vtkKWUserInterfaceManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleEntryPoints.h"
#include "KWWidgetsTourExamplePath.h"

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
  win->SetPanelLayoutToSecondaryBelowMainAndView();
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

  // Add a page, and divide it using a split frame

  source_panel->AddPage("Source", "Display the example source", NULL);
  page_widget = source_panel->GetPageWidget("Source");

  vtkKWSplitFrame *source_split = vtkKWSplitFrame::New();
  source_split->SetParent(page_widget);
  source_split->SetExpandFrameToBothFrames();
  source_split->Create(app);

  app->Script("pack %s -side top -expand y -fill both -padx 0 -pady 0", 
              source_split->GetWidgetName());

  // Add text widget to display the Tcl example source

  vtkKWTextLabeled *tcl_source_text = vtkKWTextLabeled::New();
  tcl_source_text->SetParent(source_split->GetFrame1());
  tcl_source_text->Create(app, NULL);
  tcl_source_text->SetLabelPositionToTop();
  tcl_source_text->SetLabelText("Tcl Source");

  vtkKWText *text_widget = tcl_source_text->GetWidget();
  text_widget->EditableTextOff();
  text_widget->UseVerticalScrollbarOn();
  text_widget->SetWrapToNone();
  text_widget->SetHeight(3000);
  text_widget->AddTagMatcher("#[^\n]*", "_fg_navy_tag_");
  text_widget->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text_widget->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              tcl_source_text->GetWidgetName());

  // Add text widget to display the C++ example source

  vtkKWTextLabeled *cxx_source_text = vtkKWTextLabeled::New();
  cxx_source_text->SetParent(source_split->GetFrame2());
  cxx_source_text->Create(app, NULL);
  cxx_source_text->SetLabelPositionToTop();
  cxx_source_text->SetLabelText("C++ Source");

  text_widget = cxx_source_text->GetWidget();
  text_widget->EditableTextOff();
  text_widget->UseVerticalScrollbarOn();
  text_widget->SetWrapToNone();
  text_widget->SetHeight(3000);
  text_widget->AddTagMatcher("#[a-z]+", "_fg_red_tag_");
  text_widget->AddTagMatcher("//[^\n]*", "_fg_navy_tag_");
  text_widget->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text_widget->AddTagMatcher("<[^>]*>", "_fg_blue_tag_");
  text_widget->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              cxx_source_text->GetWidgetName());

  // Populate the examples
  // Create a panel for each one, and pass the frame

  win->GetViewNotebook()->ShowOnlyPagesWithSameTagOn();

  char buffer[2048];

  WidgetNode *node_ptr = Widgets;
  while (node_ptr->Name && node_ptr->EntryPoint)
    {
    vtkKWUserInterfacePanel *panel = vtkKWUserInterfacePanel::New();
    panel->SetName(node_ptr->Name);
    panel->SetUserInterfaceManager(win->GetViewUserInterfaceManager());
    panel->Create(app);
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL, NULL);

    if ((*node_ptr->EntryPoint)(panel->GetPageWidget(panel->GetName()), win))
      {
      widgets_list->AppendUnique(node_ptr->Name);

      // Try to find the C++ source

      sprintf(
        buffer, 
        "%s/Examples/Cxx/%s/%s/%s.cxx", 
        KWWIDGETS_SOURCE_DIR, ExampleDirName, WidgetsDirName, node_ptr->Name);

      if (!vtksys::SystemTools::FileExists(buffer))
        {
        sprintf(
          buffer, 
          "%s/../share/%s/Examples/Cxx/%s/%s/%s.cxx",
          app->GetInstallationDirectory(), 
          KWWIDGETS_PROJECT_NAME,ExampleDirName,WidgetsDirName,node_ptr->Name);
        }
      if (vtksys::SystemTools::FileExists(buffer))
        {
        app->Script("set cxx_source(%s) [read [open %s]]", 
                    node_ptr->Name, buffer);
        }
      else
        {
        app->Script("set cxx_source(%s) {}", node_ptr->Name);
        }

      // Try to find the Tcl source

      sprintf(
        buffer, 
        "%s/Examples/Tcl/%s/%s/%s.tcl", 
        KWWIDGETS_SOURCE_DIR, ExampleDirName, WidgetsDirName, node_ptr->Name);

      if (!vtksys::SystemTools::FileExists(buffer))
        {
        sprintf(buffer, 
                "%s/../share/%s/Examples/Tcl/%s/%s/%s.tcl",
                app->GetInstallationDirectory(), 
                KWWIDGETS_PROJECT_NAME, 
                ExampleDirName, WidgetsDirName, node_ptr->Name);
        }
      if (vtksys::SystemTools::FileExists(buffer))
        {
        app->Script("set tcl_source(%s) [read [open %s]]", 
                    node_ptr->Name, buffer);
        }
      else
        {
        app->Script("set tcl_source(%s) {}", node_ptr->Name);
        }
      }
    node_ptr++;
    }

  // Setup the command that will raise the example panel, 
  // bring the C++ and Tcl source for each example

  sprintf(buffer, 
          "%s ShowViewUserInterface [%s GetSelection] ;"
          "%s SetValue $cxx_source([%s GetSelection]) ; "
          "%s SetValue $tcl_source([%s GetSelection]) ; ",
          win->GetTclName(),
          widgets_list->GetTclName(),
          cxx_source_text->GetWidget()->GetTclName(),
          widgets_list->GetTclName(),
          tcl_source_text->GetWidget()->GetTclName(),
          widgets_list->GetTclName());

  widgets_list->SetSingleClickCallback(NULL, buffer);
  
  // Start the application

  app->Start(argc, argv);
  int ret = app->GetExitStatus();

  // Deallocate and exit

  win->Delete();
  widgets_panel->Delete();
  widgets_list->Delete();
  source_panel->Delete();
  source_split->Delete();
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
