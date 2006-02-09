# Load the KWWidgets package

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
$app SetName "KWMedicalImageViewerExample"
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

# Create a render widget, show the corner annotation

set rw [vtkKWRenderWidget New]
$rw SetParent [$win GetViewFrame]
$rw Create
$rw CornerAnnotationVisibilityOn

pack [$rw GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

# Create a volume reader

set reader [vtkXMLImageDataReader New]
$reader SetFileName [file join [file dirname [info script]] ".." ".." Data head100x100x47.vti]

# Create an image viewer
# Use the render window and renderer of the renderwidget

set viewer [vtkImageViewer2 New]
$viewer SetRenderWindow [$rw GetRenderWindow]
$viewer SetRenderer [$rw GetRenderer]
$viewer SetInput [$reader GetOutput]
$viewer SetupInteractor [[$rw GetRenderWindow] GetInteractor]

# Reset the window/level and the camera

$reader Update
set range [[$reader GetOutput] GetScalarRange]
$viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
$viewer SetColorLevel [expr 0.5 * ([lindex $range 1] + [lindex $range 0])]

$rw ResetCamera

# The corner annotation has the ability to parse "tags" and fill
# them with information gathered from other objects.
# For example, let's display the slice and window/level in one corner
# by connecting the corner annotation to our image actor and
# image mapper

set ca [$rw GetCornerAnnotation]
$ca SetImageActor [$viewer GetImageActor]
$ca SetWindowLevel [$viewer GetWindowLevel]
$ca SetText 2 "<slice>"
$ca SetText 3 "<window>\n<level>"

# Create a scale to control the slice

set slice_scale [vtkKWScale New]
$slice_scale SetParent [$win GetViewPanelFrame]
$slice_scale Create
$slice_scale SetCommand $viewer "SetSlice"

pack [$slice_scale GetWidgetName] -side top -expand n -fill x -padx 2 -pady 2

# Create a menu button to control the orientation

set orientation_menubutton [vtkKWMenuButtonWithSpinButtonsWithLabel New]

$orientation_menubutton SetParent [$win GetMainPanelFrame]
$orientation_menubutton Create
$orientation_menubutton SetLabelText "Orientation:"
$orientation_menubutton SetPadX 2
$orientation_menubutton SetPadY 2
$orientation_menubutton SetBorderWidth 2
$orientation_menubutton SetReliefToGroove

pack [$orientation_menubutton GetWidgetName] -side top -anchor nw -expand n -fill x
             
set mb [[$orientation_menubutton GetWidget] GetWidget]
$mb AddRadioButton "X-Y" "" "$viewer SetSliceOrientationToXY ; update_slice_ranges" ""
$mb AddRadioButton "X-Z" "" "$viewer SetSliceOrientationToXZ ; update_slice_ranges" ""
$mb AddRadioButton "Y-Z" "" "$viewer SetSliceOrientationToYZ ; update_slice_ranges" ""
$mb SetValue "X-Y"

# Create a window/level preset selector

set wl_frame [vtkKWFrameWithLabel New]
$wl_frame SetParent [$win GetMainPanelFrame] 
$wl_frame Create
$wl_frame SetLabelText "Window/Level Presets"

pack [$wl_frame GetWidgetName] -side top -anchor nw -expand n -fill x -pady 2

set wl_preset_selector [vtkKWWindowLevelPresetSelector New]

$wl_preset_selector SetParent [$wl_frame GetFrame] 
$wl_preset_selector Create
$wl_preset_selector ThumbnailColumnVisibilityOn
$wl_preset_selector SetPresetApplyCommand "" "wl_preset_apply"
$wl_preset_selector SetPresetAddCommand "" "wl_preset_add"
$wl_preset_selector SetPresetUpdateCommand "" "wl_preset_update"
$wl_preset_selector SetPresetHasChangedCommand "" "wl_preset_has_changed"

pack [$wl_preset_selector GetWidgetName] -side top -anchor nw -expand n -fill x

proc wl_preset_apply {id} {
  global wl_preset_selector viewer
  if {[$wl_preset_selector HasPreset $id]} {
    $viewer SetColorWindow [$wl_preset_selector GetPresetWindow $id]
    $viewer SetColorLevel [$wl_preset_selector GetPresetLevel $id]
    $viewer Render
  }
}

proc wl_preset_add {} {
  global wl_preset_selector
  wl_preset_update [$wl_preset_selector AddPreset]
}

proc wl_preset_update {id} {
  global wl_preset_selector viewer
  $wl_preset_selector SetPresetWindow $id [$viewer GetColorWindow]
  $wl_preset_selector SetPresetLevel $id [$viewer GetColorLevel]
  wl_preset_has_changed $id
}

proc wl_preset_has_changed {id} {
  global wl_preset_selector rw
  $wl_preset_selector BuildPresetThumbnailAndScreenshotFromRenderWindow $id [$rw GetRenderWindow]
}

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
$animation_widget SetAnimationTypeToSlice
$animation_widget SetSliceSetCommand $viewer "SetSlice"
$animation_widget SetSliceGetCommand $viewer "GetSlice"

pack [$animation_widget GetWidgetName] -side top -anchor nw -expand n -fill x

proc update_slice_ranges {} { 
  global slice_scale viewer animation_widget
  $slice_scale SetRange [$viewer GetSliceMin] [$viewer GetSliceMax]
  $slice_scale SetValue [$viewer GetSlice] 
  $animation_widget SetSliceRange [$viewer GetSliceMin] [$viewer GetSliceMax]
}

update_slice_ranges

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

$rw Delete
$reader Delete
$viewer Delete
$slice_scale Delete
$orientation_menubutton Delete
$wl_frame Delete
$wl_preset_selector Delete
$animation_frame Delete
$animation_widget Delete
$win Delete
$app Delete

exit $ret
