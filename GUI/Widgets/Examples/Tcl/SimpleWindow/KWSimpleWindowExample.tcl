# Initialize Tcl

package require kwwidgets

# Process some command-line arguments

set option_test [expr [lsearch -exact $argv "--test"] == -1 ? 0 : 1]

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

vtkKWApplication app
app SetName "KWSimpleWindowExample"
if {$option_test} {
    app SetRegistryLevel 0
    app PromptBeforeExitOff
}
app RestoreApplicationSettingsFromRegistry

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://www.kitware.com"

# Add a window
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindowBase win
win SupportHelpOn
app AddWindow win
win Create app

# Add a label, attach it to the view frame, and pack

vtkKWLabel hello_label
hello_label SetParent [win GetViewFrame]
hello_label Create app
hello_label SetText "Hello, World!"
pack [hello_label GetWidgetName] -side left -anchor c -expand y
hello_label Delete

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

win Delete
app Delete

exit $ret
