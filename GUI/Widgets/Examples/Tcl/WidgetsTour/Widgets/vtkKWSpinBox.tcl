proc vtkKWSpinBoxEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a spinbox

  vtkKWSpinBox spinbox1
  spinbox1 SetParent $parent
  spinbox1 Create
  spinbox1 SetRange 0 10
  spinbox1 SetIncrement 1
  spinbox1 SetBalloonHelpString "A simple spinbox"

  pack [spinbox1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another spinbox but larger and read-only

  vtkKWSpinBox spinbox2
  spinbox2 SetParent $parent
  spinbox2 Create
  spinbox2 SetRange 10.0 15.0
  spinbox2 SetIncrement 0.5
  spinbox2 SetValue 12
  spinbox2 SetValueFormat "%.1f"
  spinbox2 SetWidth 5
  spinbox2 WrapOn
  spinbox2 SetBalloonHelpString "Another spinbox that wraps around"

  pack [spinbox2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another spinbox with a label this time

  vtkKWSpinBoxWithLabel spinbox3
  spinbox3 SetParent $parent
  spinbox3 Create
  [spinbox3 GetWidget] SetRange 10 100
  [spinbox3 GetWidget] SetIncrement 10
  spinbox3 SetLabelText "Another spinbox with a label in front:"
  spinbox3 SetBalloonHelpString \
    "This is a vtkKWSpinBoxWithLabel i.e. a spinbox associated to a\
    label that can be positioned around the spinbox."

  pack [spinbox3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  return "TypeCore"
}

proc vtkKWSpinBoxFinalizePoint {} {
  spinbox1 Delete
  spinbox2 Delete
  spinbox3 Delete
}

