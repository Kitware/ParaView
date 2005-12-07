# Initialize Tcl

package require kwwidgets

# Process some command-line arguments

set option_test [expr [lsearch -exact $argv "--test"] == -1 ? 0 : 1]

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

vtkKWApplication app
app SetName "KWWidgetsTourExample"
if {$option_test} {
  app SetRegistryLevel 0
  app PromptBeforeExitOff
}
app SupportSplashScreenOn
app SplashScreenVisibilityOn
app RestoreApplicationSettingsFromRegistry

# Setup the splash screen

[app GetSplashScreen] ReadImage [file join [file dirname [info script]] ".." ".." Resources "KWWidgetsSplashScreen.png"]

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://www.kwwidgets.org"

# Add a window to the application
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindow win
win SupportHelpOn
win SetPanelLayoutToSecondaryBelowMainAndView
app AddWindow win
win Create

# Add a user interface panel to the main user interface manager

vtkKWUserInterfacePanel widgets_panel
widgets_panel SetName "Widgets Interface"
widgets_panel SetUserInterfaceManager [win GetMainUserInterfaceManager]
widgets_panel Create

widgets_panel AddPage "Widgets" "Select a widget" ""
set page_widget [widgets_panel GetPageWidget "Widgets"]

# Add a list box to pick a widget example

vtkKWTreeWithScrollbars widgets_tree
widgets_tree SetParent $page_widget
widgets_tree VerticalScrollbarVisibilityOn
widgets_tree HorizontalScrollbarVisibilityOff
widgets_tree Create

set tree [widgets_tree GetWidget]
$tree SetPadX 0;
$tree SetBackgroundColor 1.0 1.0 1.0
$tree RedrawOnIdleOn
$tree SelectionFillOn

foreach {node text} {"core" "Core Widgets" "composite" "Composite Widgets" "vtk" "VTK Widgets"} {
  $tree AddNode "" $node $text
  $tree OpenNode $node
  $tree SetNodeSelectableFlag $node 0
  $tree SetNodeFontWeightToBold $node
}

pack [widgets_tree GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

widgets_panel Raise

# Add a user interface panel to the secondary user interface manager

vtkKWUserInterfacePanel source_panel
source_panel SetName "Source Interface"
source_panel SetUserInterfaceManager [win GetSecondaryUserInterfaceManager]
source_panel Create
[win GetSecondaryNotebook] AlwaysShowTabsOff

# Add a page, and divide it using a split frame

source_panel AddPage "Source" "Display the example source" ""
set page_widget [source_panel GetPageWidget "Source"]

vtkKWSplitFrame source_split
source_split SetParent $page_widget
source_split SetExpandableFrameToBothFrames
source_split Create

pack [source_split GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0
# Add text widget to display the Tcl example source

vtkKWTextWithScrollbarsWithLabel tcl_source_text
tcl_source_text SetParent [source_split GetFrame1]
tcl_source_text Create
tcl_source_text SetLabelPositionToTop
tcl_source_text SetLabelText "Tcl Source"

set text_widget [tcl_source_text GetWidget]
$text_widget VerticalScrollbarVisibilityOn

set text [$text_widget GetWidget]
$text ReadOnlyOn
$text SetWrapToNone
$text SetHeight 3000
$text AddTagMatcher "#\[^\n\]*" "_fg_navy_tag_"
$text AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

pack [tcl_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Add text widget to display the C++ example source

vtkKWTextWithScrollbarsWithLabel cxx_source_text
cxx_source_text SetParent [source_split GetFrame2]
cxx_source_text Create
cxx_source_text SetLabelPositionToTop
cxx_source_text SetLabelText "C++ Source"

set text_widget [cxx_source_text GetWidget]
$text_widget VerticalScrollbarVisibilityOn

set text [$text_widget GetWidget]
$text ReadOnlyOn
$text SetWrapToNone
$text SetHeight 3000
$text AddTagMatcher "#\[a-z\]+" "_fg_red_tag_"
$text AddTagMatcher "//\[^\n\]*" "_fg_navy_tag_"
$text AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text AddTagMatcher "<\[^>\]*>" "_fg_blue_tag_"
$text AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

pack [cxx_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Populate the examples
# Create a panel for each one, and pass the frame

[win GetViewNotebook] ShowOnlyPagesWithSameTagOn

set widgets [glob -path "[file join [file dirname [info script]] Widgets]/" *.tcl]
foreach widget $widgets {
  set name [file rootname [file tail $widget]]
  lappend modules $name

  set panel "panel$name"
  vtkKWUserInterfacePanel $panel
  $panel SetName $name
  $panel SetUserInterfaceManager [win GetViewUserInterfaceManager]
  $panel Create
  $panel AddPage [$panel GetName] "" ""
  lappend objects $panel

  if {[app GetSplashScreenVisibility]} {
    [app GetSplashScreen] SetProgressMessage $name
  }

  source $widget
  
  set widget_type [${name}EntryPoint [$panel GetPageWidget [$panel GetName]] win]
  if {$widget_type != ""} {
    set parent_node ""
    switch -- $widget_type {
      "TypeCore" {
        set parent_node "core"
      }
      "TypeComposite" {
        set parent_node "composite"
      }
      "TypeVTK" {
        set parent_node "vtk"
      }
    }
    [widgets_tree GetWidget] AddNode $parent_node $name $name

    set tcl_source($name) [read [open $widget]]

    # Try to find the C++ source too

    set cxx_source_name [file join [file dirname [info script]] ".." ".." Cxx WidgetsTour Widgets ${name}.cxx]
    if {[file exists $cxx_source_name]} {
      set cxx_source($name) [read [open $cxx_source_name]]
    } else {
      set cxx_source($name) {}
    }
  }
}

# Raise the example panel

proc selection_callback {} {
  global tcl_source cxx_source
  if [[widgets_tree GetWidget] HasSelection] {
    win ShowViewUserInterface [[widgets_tree GetWidget] GetSelection]
    [[tcl_source_text GetWidget] GetWidget] SetText $tcl_source([[widgets_tree GetWidget] GetSelection]) 
    [[cxx_source_text GetWidget] GetWidget] SetText $cxx_source([[widgets_tree GetWidget] GetSelection])
  } 
}

[widgets_tree GetWidget] SetSelectionChangedCommand "" selection_callback

# Start the application
# If --test was provided, do not enter the event loop

set ret 0
win Display
if {!$option_test} {
  app Start
  set ret [app GetExitStatus]
}
win Close

# Deallocate and exit

foreach name $modules { catch {${name}FinalizePoint} }

foreach object $objects { $object Delete }
win Delete
widgets_panel Delete
widgets_tree Delete
source_panel Delete
source_split Delete
tcl_source_text Delete
cxx_source_text Delete

app Delete

exit $ret
