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

# Add text widget to display the Tcl example source

source_panel AddPage "Tcl Source" "Display the Tcl example source" ""
set page_widget [source_panel GetPageWidget "Tcl Source"]

vtkKWText tcl_source_text
tcl_source_text SetParent $page_widget
tcl_source_text EditableTextOff
tcl_source_text UseVerticalScrollbarOn
tcl_source_text Create app ""
tcl_source_text SetWrapToNone

pack [tcl_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

# Add text widget to display the C++ example source

source_panel AddPage "C++ Source" "Display the C++ example source" ""
set page_widget [source_panel GetPageWidget "C++ Source"]

vtkKWText cxx_source_text
cxx_source_text SetParent $page_widget
cxx_source_text EditableTextOff
cxx_source_text UseVerticalScrollbarOn
cxx_source_text Create app ""
cxx_source_text SetWrapToNone

pack [cxx_source_text GetWidgetName] -side top -expand y -fill both -padx 2 -pady 2

source_panel RaisePage "Tcl Source"

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

set cmd {win ShowViewUserInterface [widgets_list GetSelection] ; tcl_source_text SetValue $tcl_source([widgets_list GetSelection]) ; catch { cxx_source_text SetValue $cxx_source([widgets_list GetSelection])}}

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
tcl_source_text Delete
cxx_source_text Delete

app Delete

exit $ret
