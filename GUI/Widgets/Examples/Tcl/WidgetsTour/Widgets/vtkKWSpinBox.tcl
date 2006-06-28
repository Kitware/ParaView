proc vtkKWSpinBoxEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a spinbox

  set spinbox1 [vtkKWSpinBox New]
  $spinbox1 SetParent $parent
  $spinbox1 Create
  $spinbox1 SetRange 0 10
  $spinbox1 SetIncrement 1
  $spinbox1 RestrictValuesToIntegersOn
  $spinbox1 SetBalloonHelpString "A simple spinbox"

  pack [$spinbox1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another spinbox but larger and read-only

  set spinbox2 [vtkKWSpinBox New]
  $spinbox2 SetParent $parent
  $spinbox2 Create
  $spinbox2 SetRange 10.0 15.0
  $spinbox2 SetIncrement 0.5
  $spinbox2 SetValue 12
  $spinbox2 SetValueFormat "%.1f"
  $spinbox2 SetWidth 5
  $spinbox2 WrapOn
  $spinbox2 SetBalloonHelpString "Another spinbox that wraps around"

  pack [$spinbox2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another spinbox with a label this time

  set spinbox3 [vtkKWSpinBoxWithLabel New]
  $spinbox3 SetParent $parent
  $spinbox3 Create
  [$spinbox3 GetWidget] SetRange 10 100
  [$spinbox3 GetWidget] SetIncrement 10
  $spinbox3 SetLabelText "Another spinbox with a label in front:"
  $spinbox3 SetBalloonHelpString \
    "This is a vtkKWSpinBoxWithLabel i.e. a spinbox associated to a\
    label that can be positioned around the spinbox."

  pack [$spinbox3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6
}

proc vtkKWSpinBoxGetType {} {
  return "TypeCore"
}
