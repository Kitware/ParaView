proc vtkKWExtentEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a extent

  vtkKWExtent extent1
  extent1 SetParent $parent
  extent1 Create $app
  extent1 SetBorderWidth 2
  extent1 SetReliefToGroove
  extent1 SetExtentRange 0.0 100.0 20.0 30.0 -100.0 -50.0
  extent1 SetBalloonHelpString \
    "An extent widget i.e. a set of 3 vtkKWRange that can be used to\
    control a geometric 3D extent."

  pack [extent1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another extent but put the label and entry on top

  vtkKWExtent extent2
  extent2 SetParent $parent
  extent2 Create $app
  extent2 SetBorderWidth 2
  extent2 SetReliefToGroove
  eval extent2 SetExtentRange [extent1 GetExtentRange] 
  extent2 SetLabelPositionToLeft
  extent2 SetEntry1PositionToLeft
  extent2 SetEntry2PositionToRight
  extent2 SetSliderSize 4
  extent2 SetThickness 23
  extent2 SetInternalThickness 0.7
  extent2 SetRequestedLength 200
  extent2 SetBalloonHelpString \
    "Another extent widget the label and entries are in different positions,\
    the slider and the thickness of the widget has changed and we set a\
    longer minimum length. Also note that changing this extent\
    sets the value of the first extent"

  pack [extent2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  extent2 SetCommand extent1 {SetExtent [extent2 GetExtent]}

  # -----------------------------------------------------------------------

  # Create another extent hide the last component

  vtkKWExtent extent3
  extent3 SetParent $parent
  extent3 Create $app
  extent3 SetBorderWidth 2
  extent3 SetReliefToGroove
  extent3 SetLabelPositionToRight
  extent3 SetEntry1PositionToLeft
  extent3 SetEntry2PositionToLeft
  extent3 ShowZExtentOff
  [extent3 GetXRange] SetLabelText "Horizontal"
  [extent3 GetYRange] SetLabelText "Vertical"
  extent3 SetRequestedLength 150
  extent3 SetBalloonHelpString \
    "Another extent widget but we hide the third range this can be used\
    to control a 2D extent for example. We changed the positions again."

  pack [extent3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # TODO: vertical extent

  return "TypeComposite"
}

proc vtkKWExtentFinalizePoint {} {
  extent1 Delete
  extent2 Delete
  extent3 Delete
}

