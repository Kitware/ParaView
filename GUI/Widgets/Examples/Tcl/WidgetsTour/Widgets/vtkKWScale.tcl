proc vtkKWScaleEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a scale

  vtkKWScale scale1
  scale1 SetParent $parent
  scale1 Create $app
  scale1 SetRange 0.0 100.0
  scale1 SetResolution 1.0
  scale1 SetLength 150
  scale1 DisplayEntry
  scale1 DisplayEntryAndLabelOnTopOff
  scale1 DisplayLabel "A scale:"

  pack [scale1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another scale but put the label and entry on top

  vtkKWScale scale2
  scale2 SetParent $parent
  scale2 Create $app
  scale2 SetRange 0.0 100.0
  scale2 SetResolution 1.0
  scale2 SetLength 350
  scale2 DisplayEntry
  scale2 DisplayEntryAndLabelOnTopOn
  scale2 DisplayLabel "A scale with label/entry on top:"
  scale2 SetBalloonHelpString "This time the label and entry are on top"

  pack [scale2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another scale popup mode
  # It also sets scale2 to the same value

  vtkKWScale scale3
  scale3 SetParent $parent
  scale3 PopupScaleOn
  scale3 Create $app
  scale3 SetRange 0.0 100.0
  scale3 SetResolution 1.0
  scale3 DisplayEntry
  scale3 DisplayLabel "A popup scale:"
  scale3 SetBalloonHelpString \
    "It's a pop-up and it sets the previous scale value too"

  scale3 SetCommand scale2 {SetValue [scale3 GetValue]}

  pack [scale3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a set of scale
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  vtkKWScaleSet scale_set
  scale_set SetParent $parent
  scale_set Create $app
  scale_set SetBorderWidth 2
  scale_set SetReliefToGroove
  scale_set SetMaximumNumberOfWidgetsInPackingDirection 2

  for {set id 0} {$id < 4} {incr id} {

    set scale [scale_set AddWidget $id] 
    $scale DisplayLabel "Scale $id"
    $scale SetBalloonHelpString \
      "This scale is part of a unique set a vtkKWScaleSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid."
    }

  pack [scale_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  return "TypeCore"
}

proc vtkKWScaleFinalizePoint {} {
  scale1 Delete
  scale2 Delete
  scale3 Delete
  scale_set Delete
}

