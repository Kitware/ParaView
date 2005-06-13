# Initialize Tcl

package require kwwidgets

# Create the application
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

vtkKWApplication app
app RestoreApplicationSettingsFromRegistry
app SetName "KWWidgetsTourExample"

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://www.kitware.com"

# Add a window to the application
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindow win
win SupportHelpOn
win SetPanelLayoutToSecondaryBelowMainAndView
app AddWindow win
win Create app ""

# Add a user interface panel to the main user interface manager

vtkKWUserInterfacePanel widgets_panel
widgets_panel SetName "Widgets Interface"
widgets_panel SetUserInterfaceManager [win GetMainUserInterfaceManager]
widgets_panel Create app

widgets_panel AddPage "Widgets" "Select a widget" ""
set page_widget [widgets_panel GetPageWidget "Widgets"]

# Add a list box to pick a widget example

vtkKWListBox widgets_list
widgets_list SetParent $page_widget
widgets_list ScrollbarOn
widgets_list Create app ""
widgets_list SetHeight 300

pack [widgets_list GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

widgets_panel Raise

# Add a user interface panel to the secondary user interface manager

vtkKWUserInterfacePanel source_panel
source_panel SetName "Source Interface"
source_panel SetUserInterfaceManager [win GetSecondaryUserInterfaceManager]
source_panel Create app
[win GetSecondaryNotebook] AlwaysShowTabsOff

# Add a page, and divide it using a split frame

source_panel AddPage "Source" "Display the example source" ""
set page_widget [source_panel GetPageWidget "Source"]

vtkKWSplitFrame source_split
source_split SetParent $page_widget
source_split SetExpandFrameToBothFrames
source_split Create app

pack [source_split GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0
# Add text widget to display the Tcl example source

vtkKWTextLabeled tcl_source_text
tcl_source_text SetParent [source_split GetFrame1]
tcl_source_text Create app ""
tcl_source_text SetLabelPositionToTop
tcl_source_text SetLabelText "Tcl Source"

set text_widget [tcl_source_text GetWidget]
$text_widget EditableTextOff
$text_widget UseVerticalScrollbarOn
$text_widget SetWrapToNone
$text_widget SetHeight 3000
$text_widget AddTagMatcher "#\[^\n\]*" "_fg_navy_tag_"
$text_widget AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text_widget AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

pack [tcl_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Add text widget to display the C++ example source

vtkKWTextLabeled cxx_source_text
cxx_source_text SetParent [source_split GetFrame2]
cxx_source_text Create app ""
cxx_source_text SetLabelPositionToTop
cxx_source_text SetLabelText "C++ Source"

set text_widget [cxx_source_text GetWidget]
$text_widget EditableTextOff
$text_widget UseVerticalScrollbarOn
$text_widget SetWrapToNone
$text_widget SetHeight 3000
$text_widget AddTagMatcher "#\[a-z\]+" "_fg_red_tag_"
$text_widget AddTagMatcher "//\[^\n\]*" "_fg_navy_tag_"
$text_widget AddTagMatcher "\"\[^\"\]*\"" "_fg_blue_tag_"
$text_widget AddTagMatcher "<\[^>\]*>" "_fg_blue_tag_"
$text_widget AddTagMatcher "vtk\[A-Z\]\[a-zA-Z0-9_\]+" "_fg_dark_green_tag_"

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
    $panel Create app
    $panel AddPage [$panel GetName] "" ""
    lappend objects $panel

    source $widget
    
    if {[${name}EntryPoint [$panel GetPageWidget [$panel GetName]] win]} {
        widgets_list AppendUnique $name
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

set cmd {win ShowViewUserInterface [widgets_list GetSelection] ; [tcl_source_text GetWidget] SetValue $tcl_source([widgets_list GetSelection]) ; [cxx_source_text GetWidget] SetValue $cxx_source([widgets_list GetSelection])}

widgets_list SetSingleClickCallback "" $cmd

# Start the application

app Start
set ret [app GetExitStatus]

# Deallocate and exit

foreach name $modules { catch {${name}FinalizePoint} }

foreach object $objects { $object Delete }
win Delete
widgets_panel Delete
widgets_list Delete
source_panel Delete
source_split Delete
tcl_source_text Delete
cxx_source_text Delete

app Delete

exit $ret
