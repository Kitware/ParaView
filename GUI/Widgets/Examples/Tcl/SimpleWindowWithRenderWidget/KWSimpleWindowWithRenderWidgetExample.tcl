# Initialize Tcl

package require kwwidgets
package require vtkio
package require vtkrendering

# Create the application
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

vtkKWApplication app
app RestoreApplicationSettingsFromRegistry
app SetName "KWSimpleWindowWithRenderWidgetExample"

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://www.kitware.com"

# Add a window
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindowBase win
win SupportHelpOn
app AddWindow win
win Create app ""

# Add a render widget, attach it to the view frame, and pack
  
# Create a render widget

vtkKWRenderWidget rw
rw SetParent [win GetViewFrame]
rw Create app ""

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

rw AddProp actor
rw ResetCamera

# Start the application

app Start
set ret [app GetExitStatus]

# Deallocate and exit

rw Delete
reader Delete
mapper Delete
actor Delete
win Delete
app Delete

exit $ret
