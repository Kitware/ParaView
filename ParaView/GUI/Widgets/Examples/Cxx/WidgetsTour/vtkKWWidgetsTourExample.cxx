#include "vtkKWWidgetsTourExample.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCheckButtonSet.h"
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
#include "vtkKWTkUtilities.h"

#include "vtkKWWidgetsPaths.h"

#include "KWWidgetsTourExampleEntryPoints.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/stl/map>
#include <vtksys/stl/string>

//----------------------------------------------------------------------------
class vtkKWWidgetsTourExampleInternals
{
public:

  typedef vtksys_stl::map<vtksys_stl::string, KWWidgetsTourItem*> KWWidgetsTourItemsContainer;
  typedef vtksys_stl::map<vtksys_stl::string, KWWidgetsTourItem*>::iterator KWWidgetsTourItemsContainerIterator;

  KWWidgetsTourItemsContainer KWWidgetsTourItems;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWWidgetsTourExample );
vtkCxxRevisionMacro(vtkKWWidgetsTourExample, "1.1");

//----------------------------------------------------------------------------
vtkKWWidgetsTourExample::vtkKWWidgetsTourExample()
{
  this->Internals = new vtkKWWidgetsTourExampleInternals;

  this->TclSourceText = NULL;
  this->CxxSourceText = NULL;
  this->PythonSourceText = NULL;
  this->Window = NULL;
  this->WidgetsTree = NULL;
}

//----------------------------------------------------------------------------
vtkKWWidgetsTourExample::~vtkKWWidgetsTourExample()
{
  delete this->Internals;
  this->Internals = NULL;

  this->TclSourceText->Delete();
  this->CxxSourceText->Delete();
  this->PythonSourceText->Delete();
  this->Window->Delete();
  this->WidgetsTree->Delete();
}

//----------------------------------------------------------------------------
int vtkKWWidgetsTourExample::Run(int argc, char *argv[])
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
  sprintf(res_path, 
          "%s/Resources/KWWidgetsSplashScreen.png", KWWidgets_EXAMPLES_DIR);
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

  if (!this->Window)
    {
    this->Window = vtkKWWindow::New();
    }
  this->Window->SupportHelpOn();
  this->Window->SetPanelLayoutToSecondaryBelowMainAndView();
  app->AddWindow(this->Window);
  this->Window->Create();

  // Add a user interface panel to the main user interface manager

  vtkKWUserInterfacePanel *widgets_panel = vtkKWUserInterfacePanel::New();
  widgets_panel->SetName("Widgets Interface");
  widgets_panel->SetUserInterfaceManager(
    this->Window->GetMainUserInterfaceManager());
  widgets_panel->Create();

  widgets_panel->AddPage("Widgets", "Select a widget", NULL);
  vtkKWWidget *page_widget = widgets_panel->GetPageWidget("Widgets");

  // Add a list box to pick a widget example

  if (!this->WidgetsTree)
    {
    this->WidgetsTree = vtkKWTreeWithScrollbars::New();
    }
  this->WidgetsTree->SetParent(page_widget);
  this->WidgetsTree->VerticalScrollbarVisibilityOn();
  this->WidgetsTree->HorizontalScrollbarVisibilityOn();
  this->WidgetsTree->Create();

  vtkKWTree *tree = this->WidgetsTree->GetWidget();
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
              this->WidgetsTree->GetWidgetName());

  widgets_panel->Raise();

  // Add a user interface panel to the secondary user interface manager

  vtkKWUserInterfacePanel *source_panel = vtkKWUserInterfacePanel::New();
  source_panel->SetName("Source Interface");
  source_panel->SetUserInterfaceManager(
    this->Window->GetSecondaryUserInterfaceManager());
  source_panel->Create();
  this->Window->GetSecondaryNotebook()->AlwaysShowTabsOff();

  // Add a page, and divide it using split frames

  source_panel->AddPage("Source", "Display the example source", NULL);
  page_widget = source_panel->GetPageWidget("Source");

  vtkKWSplitFrame *source_split = vtkKWSplitFrame::New();
  source_split->SetParent(page_widget);
  source_split->SetExpandableFrameToBothFrames();
  source_split->Create();

  app->Script("pack %s -side top -expand y -fill both -padx 0 -pady 0", 
              source_split->GetWidgetName());

  vtkKWSplitFrame *source_split2 = vtkKWSplitFrame::New();
  source_split2->SetParent(source_split->GetFrame2());
  source_split2->SetExpandableFrameToBothFrames();
  source_split2->Create();

  app->Script("pack %s -side top -expand y -fill both -padx 0 -pady 0", 
              source_split2->GetWidgetName());

  // Add checkbuttons to show/hide the panels

  vtkKWCheckButtonSet *panel_vis_buttons = vtkKWCheckButtonSet::New();
  panel_vis_buttons->SetParent(page_widget);
  panel_vis_buttons->PackHorizontallyOn();
  panel_vis_buttons->Create();

  vtkKWCheckButton *cb = panel_vis_buttons->AddWidget(0);
  cb->SetText("Tcl");
  cb->SetCommand(source_split, "SetFrame1Visibility");
  cb->SetSelectedState(source_split->GetFrame1Visibility());

  cb = panel_vis_buttons->AddWidget(1);
  cb->SetText("C++");
  cb->SetCommand(source_split2, "SetFrame1Visibility");
  cb->SetSelectedState(source_split2->GetFrame1Visibility());

  cb = panel_vis_buttons->AddWidget(2);
  cb->SetText("Python");
  cb->SetCommand(source_split2, "SetFrame2Visibility");
  cb->SetSelectedState(source_split2->GetFrame2Visibility());

  app->Script("pack %s -side top -anchor w", 
              panel_vis_buttons->GetWidgetName());

  // Add text widget to display the Tcl example source

  if (!this->TclSourceText)
    {
    this->TclSourceText = vtkKWTextWithScrollbarsWithLabel::New();
    }
  this->TclSourceText->SetParent(source_split->GetFrame1());
  this->TclSourceText->Create();
  this->TclSourceText->SetLabelPositionToTop();
  this->TclSourceText->SetLabelText("Tcl Source");

  vtkKWTextWithScrollbars *text_widget = this->TclSourceText->GetWidget();
  text_widget->VerticalScrollbarVisibilityOn();

  vtkKWText *text = text_widget->GetWidget();
  text->ReadOnlyOn();
  text->SetWrapToNone();
  text->SetHeight(3000);
  text->AddTagMatcher("#[^\n]*", "_fg_navy_tag_");
  text->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              this->TclSourceText->GetWidgetName());

  // Add text widget to display the C++ example source

  if (!this->CxxSourceText)
    {
    this->CxxSourceText = vtkKWTextWithScrollbarsWithLabel::New();
    }
  this->CxxSourceText->SetParent(source_split2->GetFrame1());
  this->CxxSourceText->Create();
  this->CxxSourceText->SetLabelPositionToTop();
  this->CxxSourceText->SetLabelText("C++ Source");

  text_widget = this->CxxSourceText->GetWidget();
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
              this->CxxSourceText->GetWidgetName());

  // Add text widget to display the Python++ example source

  if (!this->PythonSourceText)
    {
    this->PythonSourceText = vtkKWTextWithScrollbarsWithLabel::New();
    }
  this->PythonSourceText->SetParent(source_split2->GetFrame2());
  this->PythonSourceText->Create();
  this->PythonSourceText->SetLabelPositionToTop();
  this->PythonSourceText->SetLabelText("Python Source");

  text_widget = this->PythonSourceText->GetWidget();
  text_widget->VerticalScrollbarVisibilityOn();

  text = text_widget->GetWidget();
  text->ReadOnlyOn();
  text->SetWrapToNone();
  text->SetHeight(3000);
  text->AddTagMatcher("(\n|^| )(import|from) ", "_fg_red_tag_");
  text->AddTagMatcher("#[^\n]*", "_fg_navy_tag_");
  text->AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_");
  text->AddTagMatcher("\'[^\']*\'", "_fg_blue_tag_");
  text->AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_");

  app->Script("pack %s -side top -expand y -fill both -padx 2 -pady 2", 
              this->PythonSourceText->GetWidgetName());

  // Populate the examples
  // Create a panel for each one, and pass the frame

  this->Window->GetViewNotebook()->ShowOnlyPagesWithSameTagOn();

  KWWidgetsTourNode *node_ptr = KWWidgetsTourNodes;
  while (node_ptr->Name && node_ptr->EntryPoint)
    {
    if (app->GetSplashScreenVisibility()) 
      {
      app->GetSplashScreen()->SetProgressMessage(node_ptr->Name);
      }
    KWWidgetsTourItem *item = (*node_ptr->EntryPoint)();
    if (item)
      {
      this->Internals->KWWidgetsTourItems[node_ptr->Name] = item;
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
      this->WidgetsTree->GetWidget()->AddNode(
        parent_node, node_ptr->Name, node_ptr->Name);
      }
    node_ptr++;
    }

  this->WidgetsTree->GetWidget()->SetSelectionChangedCommand(
    this, "SelectionChangedCallback");
  
  // Start the application
  // If --test was provided, do not enter the event loop and run this example
  // as a non-interactive test for software quality purposes.

  int ret = 0;
  this->Window->Display();

  vtkKWTkUtilities::ProcessPendingEvents(app);
  source_split->SetSeparatorPosition(0.33);
  
  vtkKWWidgetsTourExampleInternals::KWWidgetsTourItemsContainerIterator end = 
    this->Internals->KWWidgetsTourItems.end();
  vtkKWWidgetsTourExampleInternals::KWWidgetsTourItemsContainerIterator it = 
    this->Internals->KWWidgetsTourItems.begin();

  if (!option_test)
    {
    app->Start(argc, argv);
    ret = app->GetExitStatus();
    }
  else
    {
    for (; it != end ; it++)
      {
      this->SelectExample((*it).first.c_str());
      }
    }
  this->Window->Close();

  // Deallocate and exit

  it = this->Internals->KWWidgetsTourItems.begin();
  for (; it != end ; it++)
    {
    delete (*it).second;
    }

  panel_vis_buttons->Delete();
  widgets_panel->Delete();
  source_panel->Delete();
  source_split->Delete();
  source_split2->Delete();
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkKWWidgetsTourExample::SelectionChangedCallback()
{
  if (this->WidgetsTree->GetWidget()->HasSelection())
    {
    vtksys_stl::string sel(this->WidgetsTree->GetWidget()->GetSelection());
    this->SelectExample(sel.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetsTourExample::SelectExample(const char *name)
{
  KWWidgetsTourItem *item = this->Internals->KWWidgetsTourItems[name];

  // If we haven't created the example, do it now

  vtkKWUserInterfacePanel *panel = 
    this->Window->GetViewUserInterfaceManager()->GetPanel(name);
  if (!panel)
    {
    panel = vtkKWUserInterfacePanel::New();
    panel->SetName(name);
    panel->SetUserInterfaceManager(
      this->Window->GetViewUserInterfaceManager());
    panel->Create();
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL, NULL);
    panel->Raise();
    item->Create(panel->GetPageWidget(panel->GetName()), this->Window);
    }
  
  this->Window->ShowViewUserInterface(name);
  vtkKWApplication *app = this->GetApplication();

  vtksys_stl::string source;
  vtksys_stl::string line;

  // Try to find the C++ source

  char buffer[1024];
  sprintf(buffer, "%s/Cxx/WidgetsTour/Widgets/%s.cxx", 
          KWWidgets_EXAMPLES_DIR, name);
  if (!vtksys::SystemTools::FileExists(buffer))
    {
    sprintf(buffer, "%s/..%s/Examples/Cxx/WidgetsTour/Widgets/%s.cxx",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR, name);
    }
  source = "";
  if (vtksys::SystemTools::FileExists(buffer))
    {
    ifstream ifs(buffer);
    while (vtksys::SystemTools::GetLineFromStream(ifs, line))
      {
      source += line;
      source += "\n";
      }
    ifs.close();
    }
  this->CxxSourceText->GetWidget()->GetWidget()->SetText(source.c_str());

  // Try to find the Tcl source

  sprintf(buffer, "%s/Tcl/WidgetsTour/Widgets/%s.tcl", 
          KWWidgets_EXAMPLES_DIR, name);
  if (!vtksys::SystemTools::FileExists(buffer))
    {
    sprintf(buffer, "%s/..%s/Examples/Tcl/WidgetsTour/Widgets/%s.tcl",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR, name);
    }
  source = "";
  if (vtksys::SystemTools::FileExists(buffer))
    {
    ifstream ifs(buffer);
    while (vtksys::SystemTools::GetLineFromStream(ifs, line))
      {
      source += line;
      source += "\n";
      }
    ifs.close();
    }
  this->TclSourceText->GetWidget()->GetWidget()->SetText(source.c_str());

  // Try to find the Python source
  
  sprintf(buffer, "%s/Python/WidgetsTour/Widgets/%s.py", 
          KWWidgets_EXAMPLES_DIR, name);
  if (!vtksys::SystemTools::FileExists(buffer))
    {
    sprintf(buffer, "%s/..%s/Examples/Python/WidgetsTour/Widgets/%s.py",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR, name);
    }
  source = "";
  if (vtksys::SystemTools::FileExists(buffer))
    {
    ifstream ifs(buffer);
    while (vtksys::SystemTools::GetLineFromStream(ifs, line))
      {
      source += line;
      source += "\n";
      }
    ifs.close();
    }
  this->PythonSourceText->GetWidget()->GetWidget()->SetText(source.c_str());
}

//----------------------------------------------------------------------------
const char* vtkKWWidgetsTourExample::GetPathToExampleData(
  vtkKWApplication *app, const char *name)
{
  static char data_path[2048];
  sprintf(
    data_path, "%s/Data/%s", KWWidgets_EXAMPLES_DIR, name);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/..%s/Examples/Data/%s",
            app->GetInstallationDirectory(), KWWidgets_INSTALL_DATA_DIR, name);
    }
  return data_path;
}
