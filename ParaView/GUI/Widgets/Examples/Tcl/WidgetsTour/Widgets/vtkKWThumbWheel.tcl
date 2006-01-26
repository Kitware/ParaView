proc vtkKWThumbWheelEntryPoint {parent win} {

  set app [$parent GetApplication]

  # -----------------------------------------------------------------------

  # Create a thumbwheel

  set thumbwheel1 [vtkKWThumbWheel New]
  $thumbwheel1 SetParent $parent
  $thumbwheel1 Create
  $thumbwheel1 SetLength 150
  $thumbwheel1 DisplayEntryOn
  $thumbwheel1 DisplayLabelOn
  [$thumbwheel1 GetLabel] SetText "A thumbwheel:"

  pack [$thumbwheel1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another thumbwheel but put the label and entry on top

  set thumbwheel2 [vtkKWThumbWheel New]
  $thumbwheel2 SetParent $parent
  $thumbwheel2 Create
  $thumbwheel2 SetRange -10.0 10.0
  $thumbwheel2 ClampMinimumValueOn
  $thumbwheel2 ClampMaximumValueOn
  $thumbwheel2 SetLength 275
  $thumbwheel2 SetSizeOfNotches [expr [$thumbwheel2 GetSizeOfNotches] * 3]
  $thumbwheel2 DisplayEntryOn
  $thumbwheel2 DisplayEntryAndLabelOnTopOn
  $thumbwheel2 DisplayLabelOn
  [$thumbwheel2 GetLabel] SetText "A thumbwheel with label/entry on top:"
  $thumbwheel2 SetBalloonHelpString "This time the label and entry are on top and we clamp the range and bigger notches"

  pack [$thumbwheel2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6
  
  # -----------------------------------------------------------------------

  # Create another thumbwheel popup mode

  set thumbwheel3 [vtkKWThumbWheel New]
  $thumbwheel3 SetParent $parent
  $thumbwheel3 PopupModeOn
  $thumbwheel3 Create
  $thumbwheel3 SetRange 0.0 100.0
  $thumbwheel3 SetResolution 1.0
  $thumbwheel3 DisplayEntryOn
  $thumbwheel3 DisplayLabelOn
  [$thumbwheel3 GetLabel] SetText "A popup thumbwheel:"

  pack [$thumbwheel3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  return "TypeComposite"
}
