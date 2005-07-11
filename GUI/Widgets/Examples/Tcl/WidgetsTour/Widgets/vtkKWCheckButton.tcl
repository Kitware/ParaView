proc vtkKWCheckButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a checkbutton

  vtkKWCheckButton cb1
  cb1 SetParent $parent
  cb1 Create $app
  cb1 SetText "A checkbutton"

  pack [cb1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another checkbutton but use an icon this time

  vtkKWCheckButton cb2
  cb2 SetParent $parent
  cb2 Create $app
  cb2 SetImageToPredefinedIcon 62
  cb2 IndicatorOff
  cb2 SetBalloonHelpString "This time use one of the predefined icon"

  pack [cb2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another checkbutton with a label this time

  vtkKWCheckButtonWithLabel cb3
  cb3 SetParent $parent
  cb3 Create $app
  cb3 SetLabelText "Another checkbutton with a label in front"
  cb3 SetBalloonHelpString \
    "This is a vtkKWCheckButtonWithLabel i.e. a checkbutton associated to a\
    label that can be positioned around the checkbutton."

  pack [cb3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a set of checkbutton
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  vtkKWCheckButtonSet cbs
  cbs SetParent $parent
  cbs Create $app
  cbs SetBorderWidth 2
  cbs SetReliefToGroove
  cbs SetMaximumNumberOfWidgetsInPackingDirection 2

  for {set i 0} {$i < 4} {incr i} {

    set cb [cbs AddWidget $i] 
    $cb SetBalloonHelpString \
      "This checkbutton is part of a unique set a vtkKWCheckButtonSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid."
    }

  # Let's be creative. The first two share the same variable name
  
  [cbs GetWidget 0] SetText "Checkbutton 0 has the same variable name as 1"
  [cbs GetWidget 1] SetText "Checkbutton 1 has the same variable name as 0"
  [cbs GetWidget 1] SetVariableName [[cbs GetWidget 0] GetVariableName]

  # The last two buttons trigger each other's states

  [cbs GetWidget 2] SetState 1
  [cbs GetWidget 2] SetText "Checkbutton 2 also toggles 3"
  [cbs GetWidget 2] SetCommand [cbs GetWidget 3] "ToggleState"

  [cbs GetWidget 3] SetText "Checkbutton 3 also toggles 2"
  [cbs GetWidget 3] SetCommand [cbs GetWidget 2] "ToggleState"
  
  pack [cbs GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # TODO: use vtkKWCheckButtonSetWithLabel

  return "TypeCore"
}

proc vtkKWCheckButtonFinalizePoint {} {
  cb1 Delete
  cb2 Delete
  cb3 Delete
  cbs Delete
}

