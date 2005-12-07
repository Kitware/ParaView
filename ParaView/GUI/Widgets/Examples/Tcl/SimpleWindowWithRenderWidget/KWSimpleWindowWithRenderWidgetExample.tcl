# Initialize Tcl

package require kwwidgets
package require vtkio
package require vtkrendering

# Process some command-line arguments

set option_test [expr [lsearch -exact $argv "--test"] == -1 ? 0 : 1]

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

vtkKWApplication app
app SetName "KWSimpleWindowWithRenderWidgetExample"
if {$option_test} {
  app SetRegistryLevel 0
  app PromptBeforeExitOff
}
app RestoreApplicationSettingsFromRegistry

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://www.kwwidgets.org"

# Add a window
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindowBase win
win SupportHelpOn
app AddWindow win
win Create

# Add a render widget, attach it to the view frame, and pack

# Create a render widget

vtkKWRenderWidget rw
rw SetParent [win GetViewFrame]
rw Create

pack [rw GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

# Switch to trackball style, it's nicer

[[[rw GetRenderWindow] GetInteractor] GetInteractorStyle] SetCurrentStyleToTrackballCamera

# Create a 3D object reader

vtkXMLPolyDataReader reader
reader SetFileName [file join [file dirname [info script]] ".." ".." Data teapot.vtp]

# Create the mapper and actor

vtkPolyDataMapper mapper
mapper SetInputConnection [reader GetOutputPort]

vtkActor actor
actor SetMapper mapper

# Add the actor to the scene

rw AddViewProp actor
rw ResetCamera

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

rw Delete
reader Delete
mapper Delete
actor Delete
win Delete
app Delete

exit $ret
