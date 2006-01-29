# Load the KWWidgets package

from kwwidgets import *

# Load requires system modules

import sys
import os
import imp
from glob import glob

# Process some command-line arguments
# The --test option here is used to run this example as a non-interactive test
# for software quality purposes. Ignore this feature in your own application.

if "--test" in sys.argv:
    option_test = 1
else:
    option_test = 0

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

def main(argv):
    
    # Initialize Tcl

    vtkKWApplication.InitializeTcl(len(argv), argv)

    # Create the application
    # If --test was provided, ignore all registry settings, and exit silently
    # Restore the settings that have been saved to the registry, like
    # the geometry of the user interface so far.

    app = vtkKWApplication()
    app.SetName("KWWidgetsTourExample")
    if option_test:
        app.SetRegistryLevel(0)
        app.PromptBeforeExitOff()

    app.SupportSplashScreenOn()
    app.SplashScreenVisibilityOn()
    app.RestoreApplicationSettingsFromRegistry()
    
    # Setup the splash screen

    app.GetSplashScreen().ReadImage(os.path.join(os.path.dirname(
                                                 os.path.abspath(__file__)),
                                                 "..","..","Resources",
                                                 "KWWidgetsSplashScreen.png"))

    # Set a help link. Can be a remote link (URL), or a local file

    app.SetHelpDialogStartingPage("http://www.kwwidgets.org")

    # Add a window to the application
    # Set 'SupportHelp' to automatically add a menu entry for the help link

    win = vtkKWWindow()
    win.SupportHelpOn()
    win.SetPanelLayoutToSecondaryBelowMainAndView()
    app.AddWindow(win)
    win.Create()

    # Add a user interface panel to the main user interface manager

    widgets_panel = vtkKWUserInterfacePanel()
    widgets_panel.SetName("Widgets Interface")
    widgets_panel.SetUserInterfaceManager(win.GetMainUserInterfaceManager())
    widgets_panel.Create()

    widgets_panel.AddPage("Widgets", "Select a widget", None)
    page_widget = widgets_panel.GetPageWidget("Widgets")

    # Add a list box to pick a widget example

    widgets_tree = vtkKWTreeWithScrollbars()
    widgets_tree.SetParent(page_widget)
    widgets_tree.VerticalScrollbarVisibilityOn()
    widgets_tree.HorizontalScrollbarVisibilityOff()
    widgets_tree.Create()

    tree = widgets_tree.GetWidget()
    tree.SetPadX(0)
    tree.SetBackgroundColor(1.0, 1.0, 1.0)
    tree.RedrawOnIdleOn()
    tree.SelectionFillOn()

    for (node,text) in [("core","Core Widgets"),
                        ("composite","Composite Widgets"),
                        ("vtk","VTK Widgets")]:
        tree.AddNode("", node, text)
        tree.OpenNode(node)
        tree.SetNodeSelectableFlag(node, 0)
        tree.SetNodeFontWeightToBold(node)

    app.Script("pack %s -side top -expand y -fill both -padx 2 -pady 2",
               widgets_tree.GetWidgetName())

    widgets_panel.Raise()

    # Add a user interface panel to the secondary user interface manager

    source_panel = vtkKWUserInterfacePanel()
    source_panel.SetName("Source Interface")
    source_panel.SetUserInterfaceManager(win.GetSecondaryUserInterfaceManager())
    source_panel.Create()
    win.GetSecondaryNotebook().AlwaysShowTabsOff()

    # Add a page, and divide it using a split frame

    source_panel.AddPage("Source", "Display the example source", None)
    page_widget = source_panel.GetPageWidget("Source")

    source_split = vtkKWSplitFrame()
    source_split.SetParent(page_widget)
    source_split.SetExpandableFrameToBothFrames()
    source_split.Create()

    app.Script("pack %s -side top -expand y -fill both -padx 0 -pady 0",
               source_split.GetWidgetName())

    # Add text widget to display the Python example source

    python_source_text = vtkKWTextWithScrollbarsWithLabel()
    python_source_text.SetParent(source_split.GetFrame1())
    python_source_text.Create()
    python_source_text.SetLabelPositionToTop()
    python_source_text.SetLabelText("Python Source")

    text_widget = python_source_text.GetWidget()
    text_widget.VerticalScrollbarVisibilityOn()

    text = text_widget.GetWidget()
    text.ReadOnlyOn()
    text.SetWrapToNone()
    text.SetHeight(3000)
    text.AddTagMatcher("(^| +)(import|from) ", "_fg_red_tag_")
    text.AddTagMatcher("#[^\n]*", "_fg_navy_tag_")
    text.AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_")
    text.AddTagMatcher("\'[^\']*\'", "_fg_blue_tag_")
    text.AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_")

    app.Script("pack %s -side top -expand y -fill both -padx 2 -pady 2",
                python_source_text.GetWidgetName())
    
    # Add text widget to display the C++ example source

    cxx_source_text = vtkKWTextWithScrollbarsWithLabel()
    cxx_source_text.SetParent(source_split.GetFrame2())
    cxx_source_text.Create()
    cxx_source_text.SetLabelPositionToTop()
    cxx_source_text.SetLabelText("C++ Source")

    text_widget = cxx_source_text.GetWidget()
    text_widget.VerticalScrollbarVisibilityOn()

    text = text_widget.GetWidget()
    text.ReadOnlyOn()
    text.SetWrapToNone()
    text.SetHeight(3000)
    text.AddTagMatcher("#[a-z]+", "_fg_red_tag_")
    text.AddTagMatcher("//[^\n]*", "_fg_navy_tag_")
    text.AddTagMatcher("\"[^\"]*\"", "_fg_blue_tag_")
    text.AddTagMatcher("<[^>]*>", "_fg_blue_tag_")
    text.AddTagMatcher("vtk[A-Z][a-zA-Z0-9_]+", "_fg_dark_green_tag_")


    app.Script("pack %s -side top -expand y -fill both -padx 2 -pady 2",
                cxx_source_text.GetWidgetName())
    
    # Populate the examples
    # Create a panel for each one, and pass the frame

    win.GetViewNotebook().ShowOnlyPagesWithSameTagOn()

    widgets = glob(os.path.join(os.path.dirname(
                                os.path.abspath(__file__)),
                                "Widgets", "*.py"))

    modules = []
    python_source = {}
    cxx_source = {}

    for widget in widgets:
        name = os.path.splitext(os.path.basename(widget))[0]
        modules.append(name)

        panel = vtkKWUserInterfacePanel()
        panel.SetName(name)
        panel.SetUserInterfaceManager(win.GetViewUserInterfaceManager())
        panel.Create()
        panel.AddPage(panel.GetName(), None, None)

        if app.GetSplashScreenVisibility():
            app.GetSplashScreen().SetProgressMessage(name)

        file,directory,desc = imp.find_module(name, [os.path.dirname(widget)])
        module = imp.load_module(name, file, directory, desc)
        file.close()

        entry_func = getattr(module, name + "EntryPoint")
        widget_type = entry_func(panel.GetPageWidget(panel.GetName()), win)
        if widget_type:
            parent_nome = None
            if widget_type == "TypeCore":
                parent_node = "core"
            elif widget_type == "TypeComposite":
                parent_node = "composite"
            elif widget_type == "TypeVTK":
                parent_node = "vtk"

            widgets_tree.GetWidget().AddNode(parent_node, name, name)

            file = open(widget, "r")
            python_source[name] = file.read()
            file.close()
            
            app.Script("set python_source(%s) {%s}",
                       name, python_source[name])

            # Try to find the C++ source too

            cxx_source_name = os.path.join(
                os.path.dirname(os.path.abspath(__file__)), "..", "..",
                "Cxx", "WidgetsTour", "Widgets", name + ".cxx")

            try:
                file = open(cxx_source_name, "r")
                cxx_source[name] = file.read()
                file.close()
            except IOError:
                print "Error"
                cxx_source[name] = ""

            app.Script("set cxx_source(%s) {%s}", name, cxx_source[name])

    # Raise the example panel

    command = ("if [%s HasSelection] {"
               % widgets_tree.GetWidget().GetTclName() +
               "%s ShowViewUserInterface [%s GetSelection] ; "
               % (win.GetTclName(), widgets_tree.GetWidget().GetTclName()) +
               "%s SetText $cxx_source([%s GetSelection]) ; "
               % (cxx_source_text.GetWidget().GetWidget().GetTclName(),
                  widgets_tree.GetWidget().GetTclName()) +
               "%s SetText $python_source([%s GetSelection]) }"
               % (python_source_text.GetWidget().GetWidget().GetTclName(),
                  widgets_tree.GetWidget().GetTclName()))

    widgets_tree.GetWidget().SetSelectionChangedCommand(None, command)

    # Start the application
    # If --test was provided, do not enter the event loop and run this example
    # as a non-interactive test for software quality purposes.

    ret = 0
    win.Display()
    if not option_test:
        app.Start(len(sys.argv), sys.argv)
        ret = app.GetExitStatus()

    win.Close()

    sys.exit(ret)

if __name__ == "__main__":
    main(sys.argv)
