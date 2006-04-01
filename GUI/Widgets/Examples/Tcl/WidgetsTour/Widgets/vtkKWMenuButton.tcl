proc vtkKWMenuButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  set days {"Monday" "Tuesday" "Wednesday" "Thursday" "Friday"}

  # -----------------------------------------------------------------------

  # Create a menu button
  # Add some entries

  set menubutton1 [vtkKWMenuButton New]
  $menubutton1 SetParent $parent
  $menubutton1 Create
  $menubutton1 SetBalloonHelpString "A simple menu button"

  for {set i 0} {$i < [llength $days]} {incr i} {

    [$menubutton1 GetMenu] AddRadioButton [lindex $days $i]
  }
  
  pack [$menubutton1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create a menu button with spin buttons

  set menubutton1b [vtkKWMenuButtonWithSpinButtons New]
  $menubutton1b SetParent $parent
  $menubutton1b Create
  [$menubutton1b GetWidget] SetWidth 20
  $menubutton1b SetBalloonHelpString \
    "This is a vtkKWMenuButtonWithSpinButtons i.e. a menu button associated\
    to a set of spin buttons vtkKWSpinButtons that can be used to\
    increment and decrement the value"

  for {set i 0} {$i < [llength $days]} {incr i} {

    [[$menubutton1b GetWidget] GetMenu] AddRadioButton [lindex $days $i]
  }

  pack [$menubutton1b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another menu button this time with a label

  set menubutton2 [vtkKWMenuButtonWithLabel New]
  $menubutton2 SetParent $parent
  $menubutton2 Create
  $menubutton2 SetBorderWidth 2
  $menubutton2 SetReliefToGroove
  $menubutton2 SetLabelText "Days:"
  $menubutton2 SetPadX 2
  $menubutton2 SetPadY 2
  [$menubutton2 GetWidget] IndicatorVisibilityOff 
  [$menubutton2 GetWidget] SetWidth 20
  $menubutton2 SetBalloonHelpString \
    "This is a vtkKWMenuButtonWithLabel i.e. a menu button associated to a\
    label that can be positioned around the menu button. The indicator is\
    hidden and the width is set explicitly"

  for {set i 0} {$i < [llength $days]} {incr i} {

    [[$menubutton2 GetWidget] GetMenu] AddRadioButton [lindex $days $i]
    }

  [$menubutton2 GetWidget] SetValue [lindex $days 0]

  pack [$menubutton2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # TODO: use callbacks
  }
  
proc vtkKWMenuButtonGetType {} {
  return "TypeCore"
}
