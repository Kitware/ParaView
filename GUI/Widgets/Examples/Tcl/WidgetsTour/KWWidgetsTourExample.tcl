# Initialize Tcl

package require kwwidgets

# Process some command-line arguments
# The --test option here is used to run this example as a non-interactive test
# for software quality purposes. Ignore this feature in your own application.

set option_test [expr [lsearch -exact $argv "--test"] == -1 ? 0 : 1]

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

set app [vtkKWApplication New]
$app SetName "KWWidgetsTourExample"
if {$option_test} {
  $app SetRegistryLevel 0
  $app PromptBeforeExitOff
}
$app SupportSplashScreenOn
$app SplashScreenVisibilityOn
$app RestoreApplicationSettingsFromRegistry

# Setup the splash screen

[$app GetSplashScreen] ReadImage [file join [file dirname [info script]] ".." ".." Resources "KWWidgetsSplashScreen.png"]

# Set a help link. Can be a remote link (URL), or a local file

$app SetHelpDialogStartingPage "http://www.kwwidgets.org"

# Add a window to the application
# Set 'SupportHelp' to automatically add a menu entry for the help link

set win [vtkKWWindow New]
$win SupportHelpOn
$win SetPanelLayoutToSecondaryBelowMainAndView
$app AddWindow $win
$win Create

# Add a user interface panel to the main user interface manager

set widgets_panel [vtkKWUserInterfacePanel New]
$widgets_panel SetName "Widgets Interface"
$widgets_panel SetUserInterfaceManager [$win GetMainUserInterfaceManager]
$widgets_panel Create

$widgets_panel AddPage "Widgets" "Select a widget" ""
set page_widget [$widgets_panel GetPageWidget "Widgets"]

# Add a list box to pick a widget example

set widgets_tree [vtkKWTreeWithScrollbars New]
$widgets_tree SetParent $page_widget
$widgets_tree VerticalScrollbarVisibilityOn
$widgets_tree HorizontalScrollbarVisibilityOff
$widgets_tree Create

set tree [$widgets_tree GetWidget]
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

pack [$widgets_tree GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

$widgets_panel Raise

# Add a user interface panel to the secondary user interface manager

set source_panel [vtkKWUserInterfacePanel New]
$source_panel SetName "Source Interface"
$source_panel SetUserInterfaceManager [$win GetSecondaryUserInterfaceManager]
$source_panel Create
[$win GetSecondaryNotebook] AlwaysShowTabsOff

# Add a page, and divide it using split frames

$source_panel AddPage "Source" "Display the example source" ""
set page_widget [$source_panel GetPageWidget "Source"]

set source_split [vtkKWSplitFrame New]
$source_split SetParent $page_widget
$source_split SetExpandableFrameToBothFrames
$source_split Create

pack [$source_split GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

set source_split2 [vtkKWSplitFrame New]
$source_split2 SetParent [$source_split GetFrame2]
$source_split2 SetExpandableFrameToBothFrames
$source_split2 Create

pack [$source_split2 GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

# Add checkbuttons to show/hide the panels

set panel_vis_buttons [vtkKWCheckButtonSet New]
$panel_vis_buttons SetParent $page_widget
$panel_vis_buttons PackHorizontallyOn 
$panel_vis_buttons Create

set cb [$panel_vis_buttons AddWidget 0]
$cb SetText "Tcl"
$cb SetCommand $source_split "SetFrame1Visibility"
$cb SetSelectedState [$source_split GetFrame1Visibility]

set cb [$panel_vis_buttons AddWidget 1]
$cb SetText "C++"
$cb SetCommand $source_split2 "SetFrame1Visibility"
$cb SetSelectedState [$source_split2 GetFrame1Visibility]

set cb [$panel_vis_buttons AddWidget 2]
$cb SetText "Python"
$cb SetCommand $source_split2 "SetFrame2Visibility"
$cb SetSelectedState [$source_split2 GetFrame2Visibility]

pack [$panel_vis_buttons GetWidgetName] -side top -anchor w

# Add text widget to display the Tcl example source

set tcl_source_text [vtkKWTextWithScrollbarsWithLabel New]
$tcl_source_text SetParent [$source_split GetFrame1]
$tcl_source_text Create
$tcl_source_text SetLabelPositionToTop
$tcl_source_text SetLabelText "Tcl Source"

set text_widget [$tcl_source_text GetWidget]
$text_widget VerticalScrollbarVisibilityOn

set text [$text_widget GetWidget]
$text ReadOnlyOn
$text SetWrapToNone
$text SetHeight 3000
$text AddTagMatcher "#\[^\n\]*" "_fg_navy_tag_"
$text AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

pack [$tcl_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Add text widget to display the C++ example source

set cxx_source_text [vtkKWTextWithScrollbarsWithLabel New]
$cxx_source_text SetParent [$source_split2 GetFrame1]
$cxx_source_text Create
$cxx_source_text SetLabelPositionToTop
$cxx_source_text SetLabelText "C++ Source"

set text_widget [$cxx_source_text GetWidget]
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

pack [$cxx_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Add text widget to display the Python example source

set python_source_text [vtkKWTextWithScrollbarsWithLabel New]
$python_source_text SetParent [$source_split2 GetFrame2]
$python_source_text Create
$python_source_text SetLabelPositionToTop
$python_source_text SetLabelText "Python Source"

set text_widget [$python_source_text GetWidget]
$text_widget VerticalScrollbarVisibilityOn

set text [$text_widget GetWidget]
$text ReadOnlyOn
$text SetWrapToNone
$text SetHeight 3000
$text AddTagMatcher "(\n|^| )(import|from) " "_fg_red_tag_"
$text AddTagMatcher "#\[^\n\]*" "_fg_navy_tag_"
$text AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text AddTagMatcher "\'\[^\'\]*\'" "_fg_blue_tag_"
$text AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

pack [$python_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Populate the examples
# Create a panel for each one, and pass the frame

[$win GetViewNotebook] ShowOnlyPagesWithSameTagOn

set widgets [glob -path "[file join [file dirname [info script]] Widgets]/" *.tcl]
foreach widget $widgets {
  set name [file rootname [file tail $widget]]
  lappend modules $name

  if {[$app GetSplashScreenVisibility]} {
    [$app GetSplashScreen] SetProgressMessage $name
  }

  source $widget

  set widget_type [${name}GetType]
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
    [$widgets_tree GetWidget] AddNode $parent_node $name $name
  }
}

[$widgets_tree GetWidget] SetSelectionChangedCommand "" selection_callback

# Raise the example panel

proc selection_callback {} {
  global tcl_source cxx_source python_source widgets_tree tcl_source_text cxx_source_text python_source_text win

  if ![[$widgets_tree GetWidget] HasSelection] {
    return
  }

  set name [[$widgets_tree GetWidget] GetSelection]
  $win ShowViewUserInterface $name
  
  set mgr [$win GetViewUserInterfaceManager]
  
  if {"[$mgr GetPanel $name]" == ""} {
    set panel [vtkKWUserInterfacePanel New]
    $panel SetName $name
    $panel SetUserInterfaceManager $mgr
    $panel Create
    $panel AddPage [$panel GetName] "" ""
    $panel Raise
    ${name}EntryPoint [$panel GetPageWidget [$panel GetName]] $win
  }

  # Try to find the Tcl source
  
  set source {}
  set tcl_source_name [file join [file dirname [info script]] ".." ".." Tcl WidgetsTour Widgets ${name}.tcl]
  if {[file exists $tcl_source_name]} {
    set source [read [open "$tcl_source_name"]]
  }
  [[$tcl_source_text GetWidget] GetWidget] SetText $source

  # Try to find the C++ source
  
  set source {}
  set cxx_source_name [file join [file dirname [info script]] ".." ".." Cxx WidgetsTour Widgets ${name}.cxx]
  if {[file exists $cxx_source_name]} {
    set source [read [open "$cxx_source_name"]]
  }
  [[$cxx_source_text GetWidget] GetWidget] SetText $source
  
  # Try to find the Python source
  
  set source {}
  set python_source_name [file join [file dirname [info script]] ".." ".." Python WidgetsTour Widgets ${name}.py]
  if {[file exists $python_source_name]} {
    set source [read [open "$python_source_name"]]
  }
  [[$python_source_text GetWidget] GetWidget] SetText $source
} 

# Start the application
# If --test was provided, do not enter the event loop and run this example
# as a non-interactive test for software quality purposes.

set ret 0
$win Display

update
$source_split SetSeparatorPosition 0.33

if {!$option_test} {
  $app Start
  set ret [$app GetExitStatus]
}
$win Close

# Deallocate and exit

# A few objects need to be deleted first

foreach class {vtkImageViewer2 vtkKWRenderWidget} {
  foreach obj [$class ListInstances] {
    $obj Delete
  }
}

$win Delete

$panel_vis_buttons Delete
$widgets_panel Delete
$widgets_tree Delete
$source_panel Delete
$source_split Delete
$source_split2 Delete
$tcl_source_text Delete
$cxx_source_text Delete
$python_source_text Delete
$app Delete

# And delete all the remaining objects that were created by each example

vtkCommand DeleteAllObjects

exit $ret
