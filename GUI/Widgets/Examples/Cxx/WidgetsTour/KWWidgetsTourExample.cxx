#include "vtkKWApplication.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWTree.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkKWNotebook.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWTextWithScrollbarsWithLabel.h"
#include "vtkKWUserInterfaceManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsConfigurePaths.h"

#include "KWWidgetsTourExampleEntryPoints.h"
#include "KWWidgetsTourExamplePath.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/stl/vector>

typedef vtksys_stl::vector<KWWidgetsTourItem*> KWWidgetsTourItemsContainer;
typedef vtksys_stl::vector<KWWidgetsTourItem*>::iterator KWWidgetsTourItemsContainerIterator;

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
  app->SetName("KWWidgetsTourExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    app->PromptBeforeExitOff();
    }
  app->SupportSplashScreenOn();
  app->SplashScreenVisibilityOn();
  app->RestoreApplicationSettingsFromRegistry();

  // Setup the splash screen

  char res_path[2048];
  sprintf(res_path, "%s/Examples/Resources/KWWidgetsSplashScreen.png", 
          KWWidgets_SOURCE_DIR);
  if (!vtksys::SystemTools::FileExists(res_path))
    {
    sprintf(res_path, 
            "%s/..%s/Examples/Resources/KWWidgetsSplashScreen.png",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR);
    }
  app->GetSplashScreen()->ReadImage(res_path);

  // Set a help link. Can be a remote link (URL), or a local file

  app->SetHelpDialogStartingPage("http://www.kwwidgets.org");

  // Add a window to the application
  // Set 'SupportHelp' to automatically add a menu entry for the help link

  vtkKWWindow *win = vtkKWWindow::New();
  win->SupportHelpOn();
  win->SetPanelLayoutToSecondaryBelowMainAndView();
  app->AddWindow(win);
  win->Create();

  // Add a user interface panel to the main user interface manager

  vtkKWUserInterfacePanel *widgets_panel = vtkKWUserInterfacePanel::New();
  widgets_panel->SetName("Widgets Interface");
  widgets_panel->SetUserInterfaceManager(
    win->GetMainUserInterfaceManager());
  widgets_panel->Create();

  widgets_panel->AddPage("Widgets", "Select a widget", NULL);
  vtkKWWidget *page_widget = widgets_panel->GetPageWidget("Widgets");

  // Add a list box to pick a widget example

  vtkKWTreeWithScrollbars *widgets_tree = vtkKWTreeWithScrollbars::New();
  widgets_tree->SetParent(page_widget);
  widgets_tree->VerticalScrollbarVisibilityOn();
  widgets_tree->HorizontalScrollbarVisibilityOn();
  widgets_tree->Create();

  vtkKWTree *tree = widgets_tree->GetWidget();
  tree->SetPadX(0);
  tree->SetBackgroundColor(1.0, 1.0, 1.0);
  tree->RedrawOnIdleOn();
  tree->SelectionFillOn();

  tree->AddNode(NULL, "core", "Core Widgets");
  tree->OpenNode("core");
  tree->SetNodeSelectableFlag("core", 0);
  tree->SetNodeFontWeightToBold("core");

  tree->AddNode(NULL, "composite", "Composite Widgets");
  tree->OpenNode("composite");
  tree->SetNodeSelectableFlag("composite", 0);
  tree->SetNodeFontWeightToBold("composite");

  tree->AddNode(NULL, "vtk", "VTK Widgets");
  tree->OpenNode("vtk");
  tree->SetNodeSelectableFlag("vtk", 0);
  tree->SetNodeFontWeightToBold("vtk");
 
  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              widgets_tree->GetWidgetName());

  widgets_panel->Raise();

  // Add a user interface panel to the secondary user interface manager

  vtkKWUserInterfacePanel *source_panel = vtkKWUserInterfacePanel::New();
  source_panel->SetName("Source Interface");
  source_panel->SetUserInterfaceManager(
    win->GetSecondaryUserInterfaceManager());
  source_panel->Create();
  win->GetSecondaryNotebook()->AlwaysShowTabsOff();

  // Add a page, and divide it using a split frame

  source_panel->AddPage("Source", "Display the example source", NULL);
  page_widget = source_panel->GetPageWidget("Source");

  vtkKWSplitFrame *source_split = vtkKWSplitFrame::New();
  source_split->SetParent(page_widget);
  source_split->SetExpandableFrameToBothFrames();
  source_split->Create();

  app->Script("pack %s -side top -expand y -fill both -padx 0 -pady 0", 
              source_split->GetWidgetName());

  // Add text widget to display the Tcl example source

  vtkKWTextWithScrollbarsWithLabel *tcl_source_text = 
    vtkKWTextWithScrollbarsWithLabel::New();
  tcl_source_text->SetParent(source_split->GetFrame1());
  tcl_source_text->Create();
  tcl_source_text->SetLabelPositionToTop();
  tcl_source_text->SetLabelText("Tcl Source");

  vtkKWTextWithScrollbars *text_widget = tcl_source_text->GetWidget();
  text_widget->VerticalScrollbarVisibilityOn();

  vtkKWText *text = text_widget->GetWidget();
  text->ReadOnlyOn();
  text->SetWrapToNone();
  text->SetHeight(3000);
  text->AddTagMatcher("#[^\n]*", "_fg_navy_tag_");
  text->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              tcl_source_text->GetWidgetName());

  // Add text widget to display the C++ example source

  vtkKWTextWithScrollbarsWithLabel *cxx_source_text = 
    vtkKWTextWithScrollbarsWithLabel::New();
  cxx_source_text->SetParent(source_split->GetFrame2());
  cxx_source_text->Create();
  cxx_source_text->SetLabelPositionToTop();
  cxx_source_text->SetLabelText("C++ Source");

  text_widget = cxx_source_text->GetWidget();
  text_widget->VerticalScrollbarVisibilityOn();

  text = text_widget->GetWidget();
  text->ReadOnlyOn();
  text->SetWrapToNone();
  text->SetHeight(3000);
  text->AddTagMatcher("#[a-z]+", "_fg_red_tag_");
  text->AddTagMatcher("//[^\n]*", "_fg_navy_tag_");
  text->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text->AddTagMatcher("<[^>]*>", "_fg_blue_tag_");
  text->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              cxx_source_text->GetWidgetName());

  // Populate the examples
  // Create a panel for each one, and pass the frame

  win->GetViewNotebook()->ShowOnlyPagesWithSameTagOn();

  char buffer[2048];

  KWWidgetsTourItemsContainer KWWidgetsTourItems;

  KWWidgetsTourNode *node_ptr = KWWidgetsTourNodes;
  while (node_ptr->Name && node_ptr->EntryPoint)
    {
    vtkKWUserInterfacePanel *panel = vtkKWUserInterfacePanel::New();
    panel->SetName(node_ptr->Name);
    panel->SetUserInterfaceManager(win->GetViewUserInterfaceManager());
    panel->Create();
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL, NULL);

    if (app->GetSplashScreenVisibility()) 
      {
      app->GetSplashScreen()->SetProgressMessage(node_ptr->Name);
      }

    KWWidgetsTourItem *item = 
      (*node_ptr->EntryPoint)(panel->GetPageWidget(panel->GetName()), win);
    if (item)
      {
      KWWidgetsTourItems.push_back(item);
      const char *parent_node = NULL;
      switch (item->GetType())
        {
        default:
        case KWWidgetsTourItem::TypeCore:
          parent_node = "core";
          break;
        case KWWidgetsTourItem::TypeComposite:
          parent_node = "composite";
          break;
        case KWWidgetsTourItem::TypeVTK:
          parent_node = "vtk";
          break;
        }
      widgets_tree->GetWidget()->AddNode(
        parent_node, node_ptr->Name, node_ptr->Name);

      // Try to find the C++ source

      sprintf(
        buffer, 
        "%s/Examples/Cxx/%s/%s/%s.cxx", 
        KWWidgets_SOURCE_DIR, ExampleDirName, WidgetsDirName, node_ptr->Name);

      if (!vtksys::SystemTools::FileExists(buffer))
        {
        sprintf(
          buffer, 
          "%s/..%s/Examples/Cxx/%s/%s/%s.cxx",
          app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR,
          ExampleDirName, WidgetsDirName, node_ptr->Name);
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
        KWWidgets_SOURCE_DIR, ExampleDirName, WidgetsDirName, node_ptr->Name);

      if (!vtksys::SystemTools::FileExists(buffer))
        {
        sprintf(buffer, 
                "%s/..%s/Examples/Tcl/%s/%s/%s.tcl",
                app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR,
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
          "if [%s HasSelection] {"
          "%s ShowViewUserInterface [%s GetSelection] ; "
          "%s SetText $cxx_source([%s GetSelection]) ; "
          "%s SetText $tcl_source([%s GetSelection]) }",
          widgets_tree->GetWidget()->GetTclName(),
          win->GetTclName(),
          widgets_tree->GetWidget()->GetTclName(),
          cxx_source_text->GetWidget()->GetWidget()->GetTclName(),
          widgets_tree->GetWidget()->GetTclName(),
          tcl_source_text->GetWidget()->GetWidget()->GetTclName(),
          widgets_tree->GetWidget()->GetTclName());

  widgets_tree->GetWidget()->SetSelectionChangedCommand(NULL, buffer);
  
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

  KWWidgetsTourItemsContainerIterator it = KWWidgetsTourItems.begin();
  KWWidgetsTourItemsContainerIterator end = KWWidgetsTourItems.end();
  for (; it != end ; it++)
    {
    delete (*it);
    }

  win->Delete();
  widgets_panel->Delete();
  widgets_tree->Delete();
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

const char* KWWidgetsTourItem::GetPathToExampleData(
  vtkKWApplication *app, const char *name)
{
  static char data_path[2048];
  sprintf(
    data_path, "%s/Examples/Data/%s", KWWidgets_SOURCE_DIR, name);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/..%s/Examples/Data/%s",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR, name);
    }
  return data_path;
}
