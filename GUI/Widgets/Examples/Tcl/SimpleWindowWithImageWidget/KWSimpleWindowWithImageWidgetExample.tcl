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
app SetName "KWSimpleWindowWithImageWidgetExample"
if {$option_test} {
    app SetRegistryLevel 0
    app PromptBeforeExitOff
}
app RestoreApplicationSettingsFromRegistry

# Set a help link. Can be a remote link (URL), or a local file

app SetHelpDialogStartingPage "http://public.kitware.com/KWWidgets"

# Add a window
# Set 'SupportHelp' to automatically add a menu entry for the help link

vtkKWWindowBase win
win SupportHelpOn
app AddWindow win
win Create app

# Add a render widget, attach it to the view frame, and pack
  
# Create a render widget

vtkKWRenderWidget rw
rw SetParent [win GetViewFrame]
rw Create app

pack [rw GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

# Create a volume reader

vtkXMLImageDataReader reader
reader SetFileName [file join [file dirname [info script]] ".." ".." Data head100x100x47.vti]

# Create an image viewer
# Use the render window and renderer of the renderwidget

vtkImageViewer2 viewer
viewer SetRenderWindow [rw GetRenderWindow]
viewer SetRenderer [rw GetRenderer]
viewer SetInput [reader GetOutput]

vtkRenderWindowInteractor iren
viewer SetupInteractor iren

# Reset the window/level and the camera

reader Update
set range [[reader GetOutput] GetScalarRange]
viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
viewer SetColorLevel [expr 0.5 * ([lindex $range 1] + [lindex $range 0])]

rw ResetCamera

# Create a scale to control the slice

vtkKWScale slice_scale
slice_scale SetParent [win GetViewFrame]
slice_scale Create app
slice_scale DisplayEntry
slice_scale DisplayEntryAndLabelOnTopOff
slice_scale DisplayLabel "Slice:"
slice_scale SetRange [viewer GetWholeZMin] [viewer GetWholeZMax]
slice_scale SetValue [viewer GetZSlice]
slice_scale SetCommand "" {viewer SetZSlice [slice_scale GetValue] ; rw Render}

pack [slice_scale GetWidgetName] -side top -expand n -fill x -padx 2 -pady 2

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
viewer Delete
iren Delete
slice_scale Delete
win Delete
app Delete

exit $ret
