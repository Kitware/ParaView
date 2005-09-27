proc vtkKWChangeColorButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a color button. The label is inside the button

  vtkKWChangeColorButton ccb1
  ccb1 SetParent $parent
  ccb1 Create $app
  ccb1 SetColor 1.0 0.0 0.0
  ccb1 SetLabelPositionToLeft
  ccb1 SetLabelText "Set Background Color"
  ccb1 SetCommand $parent "SetBackgroundColor"
  eval ccb1 SetColor [$parent GetBackgroundColor] 
  ccb1 SetBalloonHelpString \
    "A color button. Note that the label is inside the button. Its position\
    can be changed. It sets the background color of its parent."

  pack [ccb1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another ccb but put the label and entry on top

  vtkKWChangeColorButton ccb2
  ccb2 SetParent $parent
  ccb2 Create $app
  ccb2 SetColor 0.0 1.0 0.0
  ccb2 LabelOutsideButtonOn
  ccb2 SetLabelPositionToRight
  ccb2 SetCommand [ccb2 GetLabel] "SetForegroundColor"
  eval ccb2 SetColor [[ccb2 GetLabel] GetForegroundColor]
  ccb2 SetBalloonHelpString \
    "A color button. Note that the label is now outside the button and its\
    default position has been changed to the right. It sets the color\
    of its own internal label."
    
  pack [ccb2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another color button without a label

  vtkKWChangeColorButton ccb3
  ccb3 SetParent $parent
  ccb3 Create $app
  ccb3 SetColor 0.0 0.0 1.0
  ccb3 LabelVisibilityOff
  ccb3 SetBalloonHelpString \
    "A color button. Note that the label is now hidden."

  pack [ccb3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  return "TypeComposite"
}

proc vtkKWChangeColorButtonFinalizePoint {} {
  ccb1 Delete
  ccb2 Delete
  ccb3 Delete
}

