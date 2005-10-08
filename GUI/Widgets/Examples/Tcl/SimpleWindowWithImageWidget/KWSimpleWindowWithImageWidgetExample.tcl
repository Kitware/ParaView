# Initialize Tcl

package require kwwidgets

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

vtkKWWindow win
win SupportHelpOn
app AddWindow win
win Create app
win SecondaryPanelVisibilityOff

# Add a render widget, attach it to the view frame, and pack

# Create a render widget, show the corner annotation

vtkKWRenderWidget rw
rw SetParent [win GetViewFrame]
rw Create app
rw CornerAnnotationVisibilityOn

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
viewer SetupInteractor [[rw GetRenderWindow] GetInteractor]

# Reset the window/level and the camera

reader Update
set range [[reader GetOutput] GetScalarRange]
viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
viewer SetColorLevel [expr 0.5 * ([lindex $range 1] + [lindex $range 0])]

rw ResetCamera

# The corner annotation has the ability to parse "tags" and fill
# them with information gathered from other objects.
# For example, let's display the slice and window/level in one corner
# by connecting the corner annotation to our image actor and
# image mapper

set ca [rw GetCornerAnnotation]
$ca SetImageActor [viewer GetImageActor]
$ca SetWindowLevel [viewer GetWindowLevel]
$ca SetText 2 "<slice>"
$ca SetText 3 "<window>\n<level>"

# Create a scale to control the slice

vtkKWScale slice_scale
slice_scale SetParent [win GetViewPanelFrame]
slice_scale Create app
slice_scale SetCommand "" {viewer SetSlice [slice_scale GetValue]}

proc update_slice_scale {} { 
  slice_scale SetRange [viewer GetSliceMin] [viewer GetSliceMax]
  slice_scale SetValue [viewer GetSlice] 
}

update_slice_scale

pack [slice_scale GetWidgetName] -side top -expand n -fill x -padx 2 -pady 2

# Create a menu button to control the orientation

vtkKWMenuButtonWithSpinButtonsWithLabel orientation_menubutton

orientation_menubutton SetParent [win GetMainPanelFrame]
orientation_menubutton Create app
orientation_menubutton SetLabelText "Orientation:"
orientation_menubutton SetPadX 2
orientation_menubutton SetPadY 2
orientation_menubutton SetBorderWidth 2
orientation_menubutton SetReliefToGroove

pack [orientation_menubutton GetWidgetName] -side top -anchor nw -expand n -fill x
             
set mb [[orientation_menubutton GetWidget] GetWidget]
$mb AddRadioButton "X-Y" "" "viewer SetSliceOrientationToXY ; update_slice_scale" ""
$mb AddRadioButton "X-Z" "" "viewer SetSliceOrientationToXZ ; update_slice_scale" ""
$mb AddRadioButton "Y-Z" "" "viewer SetSliceOrientationToYZ ; update_slice_scale" ""
$mb SetValue "X-Y"

# Create a window/level preset selector

vtkKWFrameWithLabel wl_frame
wl_frame SetParent [win GetMainPanelFrame] 
wl_frame Create app
wl_frame SetLabelText "Window/Level Presets"

pack [wl_frame GetWidgetName] -side top -anchor nw -expand n -fill x -pady 2

vtkKWWindowLevelPresetSelector wl_preset_selector

wl_preset_selector SetParent [wl_frame GetFrame] 
wl_preset_selector Create app
wl_preset_selector SetPresetApplyCommand "" "wl_preset_apply"
wl_preset_selector SetPresetAddCommand "" "wl_preset_add"
wl_preset_selector SetPresetUpdateCommand "" "wl_preset_update"
wl_preset_selector SetPresetHasChangedCommand "" "wl_preset_has_changed"

pack [wl_preset_selector GetWidgetName] -side top -anchor nw -expand n -fill x

proc wl_preset_apply {id} {
  if {[wl_preset_selector HasPreset $id]} {
    viewer SetColorWindow [wl_preset_selector GetPresetWindow $id]
    viewer SetColorLevel [wl_preset_selector GetPresetLevel $id]
    viewer Render
  }
}

proc wl_preset_add {} {
  wl_preset_update [wl_preset_selector AddPreset]
}

proc wl_preset_update {id} {
  wl_preset_selector SetPresetWindow $id [viewer GetColorWindow]
  wl_preset_selector SetPresetLevel $id [viewer GetColorLevel]
  wl_preset_has_changed $id
}

proc wl_preset_has_changed {id} {
  wl_preset_selector SetPresetImageFromRenderWindow $id [rw GetRenderWindow]
}

# Create a simple animation widget

vtkKWFrameWithLabel animation_frame
animation_frame SetParent [win GetMainPanelFrame] 
animation_frame Create app
animation_frame SetLabelText "Movie Creator"

pack [animation_frame GetWidgetName] -side top -anchor nw -expand n -fill x -pady 2

vtkKWSimpleAnimationWidget animation_widget
animation_widget SetParent [animation_frame GetFrame] 
animation_widget Create app
animation_widget SetRenderWidget rw
animation_widget SetAnimationTypeToSlice
animation_widget SetSliceSetCommand viewer "SetSlice"
animation_widget SetSliceGetCommand viewer "GetSlice"
animation_widget SetSliceGetMinAndMaxCommands viewer "GetSliceMin" "GetSliceMax"

pack [animation_widget GetWidgetName] -side top -anchor nw -expand n -fill x

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
slice_scale Delete
orientation_menubutton Delete
wl_frame Delete
wl_preset_selector Delete
animation_frame Delete
animation_widget Delete
win Delete
app Delete

exit $ret
