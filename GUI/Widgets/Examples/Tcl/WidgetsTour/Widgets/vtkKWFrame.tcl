proc vtkKWFrameEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a frame

  set frame1 [vtkKWFrame New]
  $frame1 SetParent $parent
  $frame1 Create
  $frame1 SetWidth 200
  $frame1 SetHeight 50
  $frame1 SetBackgroundColor 0.5 0.5 0.95
  $frame1 SetBalloonHelpString \
    "Another frame set its size explicitly and change its color"

  pack [$frame1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a labeled frame

  set frame2 [vtkKWFrameWithLabel New]
  $frame2 SetParent $parent
  $frame2 Create
  $frame2 SetLabelText "A Labeled Frame"
  $frame2 SetWidth 300
  $frame2 SetHeight 100
  $frame2 SetBalloonHelpString \
    "This is a vtkKWFrameWithLabel i.e. a frame associated to a\
    label on top of it that can be collapsed or expanded. Its size is\
    set explicitly here but should adjust automatically otherwise"

  pack [$frame2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6
}

proc vtkKWFrameGetType {} {
  return "TypeCore"
}
