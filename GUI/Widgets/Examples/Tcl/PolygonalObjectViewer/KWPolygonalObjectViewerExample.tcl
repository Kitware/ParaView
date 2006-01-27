# Load the KWWidgets package

package require kwwidgets
package require vtkio
package require vtkrendering

# Process some command-line arguments
# The --test option here is used to run this example as a non-interactive test
# for software quality purposes. Ignore this feature in your own application.

set option_test [expr [lsearch -exact $argv "--test"] == -1 ? 0 : 1]

# Create the application
# If --test was provided, ignore all registry settings, and exit silently
# Restore the settings that have been saved to the registry, like
# the geometry of the user interface so far.

set app [vtkKWApplication New]
$app SetName "KWPolygonalObjectViewerExample"
if {$option_test} {
  $app SetRegistryLevel 0
  $app PromptBeforeExitOff
}
$app RestoreApplicationSettingsFromRegistry

# Set a help link. Can be a remote link (URL), or a local file

$app SetHelpDialogStartingPage "http://www.kwwidgets.org"

# Add a window
# Set 'SupportHelp' to automatically add a menu entry for the help link

set win [vtkKWWindow New]
$win SupportHelpOn
$app AddWindow $win
$win Create
$win SecondaryPanelVisibilityOff

# Add a render widget, attach it to the view frame, and pack

# Create a render widget

set rw [vtkKWRenderWidget New]
$rw SetParent [$win GetViewFrame]
$rw Create

pack [$rw GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

# Create a 3D object reader

set reader [vtkXMLPolyDataReader New]
$reader SetFileName [file join [file dirname [info script]] ".." ".." Data teapot.vtp]

# Create the mapper and actor

set mapper [vtkPolyDataMapper New]
$mapper SetInputConnection [$reader GetOutputPort]

set actor [vtkActor New]
$actor SetMapper $mapper

# Add the actor to the scene

$rw AddViewProp $actor
$rw ResetCamera

# Create a material property editor

set mat_prop_widget [vtkKWSurfaceMaterialPropertyWidget New]
$mat_prop_widget SetParent [$win GetMainPanelFrame]
$mat_prop_widget Create
$mat_prop_widget SetPropertyChangedCommand $rw "Render"
$mat_prop_widget SetPropertyChangingCommand $rw "Render"

$mat_prop_widget SetProperty [$actor GetProperty]

pack [$mat_prop_widget GetWidgetName] -side top -anchor nw -expand n -fill x

# Create a simple animation widget

set animation_frame [vtkKWFrameWithLabel New]
$animation_frame SetParent [$win GetMainPanelFrame] 
$animation_frame Create
$animation_frame SetLabelText "Movie Creator"

pack [$animation_frame GetWidgetName] -side top -anchor nw -expand n -fill x -pady 2

set animation_widget [vtkKWSimpleAnimationWidget New]
$animation_widget SetParent [$animation_frame GetFrame] 
$animation_widget Create
$animation_widget SetRenderWidget $rw
$animation_widget SetAnimationTypeToCamera

pack [$animation_widget GetWidgetName] -side top -anchor nw -expand n -fill x

# Start the application
# If --test was provided, do not enter the event loop and run this example
# as a non-interactive test for software quality purposes.

set ret 0
$win Display
if {!$option_test} {
  $app Start
  set ret [$app GetExitStatus]
}
$win Close

# Deallocate and exit

$mat_prop_widget Delete
$animation_frame Delete
$animation_widget Delete
$rw Delete
$reader Delete
$mapper Delete
$actor Delete
$win Delete
$app Delete

exit $ret
