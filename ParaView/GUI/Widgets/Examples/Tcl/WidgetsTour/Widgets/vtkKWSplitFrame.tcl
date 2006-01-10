proc vtkKWSplitFrameEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a split frame

  vtkKWSplitFrame splitframe1
  splitframe1 SetParent $parent
  splitframe1 Create
  splitframe1 SetWidth 400
  splitframe1 SetHeight 200
  splitframe1 SetReliefToGroove
  splitframe1 SetBorderWidth 2
  splitframe1 SetExpandableFrameToBothFrames
  splitframe1 SetFrame1MinimumSize 5
  splitframe1 SetFrame2MinimumSize 5

  pack [splitframe1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # Change the color of each pane

  [splitframe1 GetFrame1] SetBackgroundColor 0.2 0.2 0.95
  [splitframe1 GetFrame2] SetBackgroundColor 0.95 0.2 0.2

  return "TypeComposite"
}

proc vtkKWSplitFrameFinalizePoint {} {
  splitframe1 Delete
}

