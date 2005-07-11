proc vtkKWMenuButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  set days {"Monday" "Tuesday" "Wednesday" "Thursday" "Friday"}

  # -----------------------------------------------------------------------

  # Create a menu button
  # Add some entries

  vtkKWMenuButton menubutton1
  menubutton1 SetParent $parent
  menubutton1 Create $app
  menubutton1 SetBalloonHelpString "A simple menu button"

  for {set i 0} {$i < [llength $days]} {incr i} {

    menubutton1 AddRadioButton [lindex $days $i]
  }

  pack [menubutton1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another menu button this time with a label

  vtkKWMenuButtonWithLabel menubutton2
  menubutton2 SetParent $parent
  menubutton2 Create $app
  menubutton2 SetBorderWidth 2
  menubutton2 SetReliefToGroove
  menubutton2 SetLabelText "Days:"
  menubutton2 SetPadX 2
  menubutton2 SetPadY 2
  [menubutton2 GetWidget] IndicatorOff 
  [menubutton2 GetWidget] SetWidth 20
  menubutton2 SetBalloonHelpString \
    "This is a vtkKWMenuButtonWithLabel i.e. a menu button associated to a\
    label that can be positioned around the menu button. The indicator is\
    hidden and the width is set explicitly"

  for {set i 0} {$i < [llength $days]} {incr i} {

    [menubutton2 GetWidget] AddRadioButton [lindex $days $i]
    }

  [menubutton2 GetWidget] SetValue [lindex $days 0]

  pack [menubutton2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # TODO: use callbacks

  return "TypeCore"
}

proc vtkKWMenuButtonFinalizePoint {} {
  menubutton1 Delete
  menubutton2 Delete
}

