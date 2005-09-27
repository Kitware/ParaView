proc vtkKWCheckButtonWithChangeColorButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a checkbutton with change color button

  vtkKWCheckButtonWithChangeColorButton cbwcc1
  cbwcc1 SetParent $parent
  cbwcc1 Create $app
  [cbwcc1 GetCheckButton] SetText "a checkbutton with color change button"
  [cbwcc1 GetChangeColorButton] SetColor 0.1 0.3 0.9
  cbwcc1 SetBalloonHelpString \
    "A checkbutton vtkKWCheckButton associated to a color change button\
     vtkKWChangeColorButton. A typical use for this widget is to control\
    the visibility of something through the checkbutton and set its color\
    through the change color button."

  pack [cbwcc1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another checkbutton with change color button

  vtkKWCheckButtonWithChangeColorButton cbwcc2
  cbwcc2 SetParent $parent
  cbwcc2 Create $app
  cbwcc2 SetBorderWidth 2
  cbwcc2 SetReliefToGroove
  [cbwcc2 GetCheckButton] SetText "Another one"
  [cbwcc2 GetChangeColorButton] LabelVisibilityOff 
  [cbwcc2 GetChangeColorButton] SetColor 0.9 0.3 0.1
  cbwcc2 DisableChangeColorButtonWhenNotCheckedOn
  cbwcc2 SetBalloonHelpString \
    "Another one this time with a border showing that we are indeed dealing\
    with a composite widget. Note that the color change button is disabled\
    automatically if the checkbutton is not set in this example. This\
    allows the dependent control color to be disabled if the main one\
     visibility for example is disabled for example if an object is not\
    don't bother setting its color..."

  pack [cbwcc2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  return "TypeComposite"
}

proc vtkKWCheckButtonWithChangeColorButtonFinalizePoint {} {
  cbwcc1 Delete
  cbwcc2 Delete
}

