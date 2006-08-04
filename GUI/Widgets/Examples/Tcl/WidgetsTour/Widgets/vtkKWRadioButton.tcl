proc vtkKWRadioButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create two radiobuttons. 
  # They share the same variable name and each one has a different internal
  # value.

  set radiob1 [vtkKWRadioButton New]
  $radiob1 SetParent $parent
  $radiob1 Create
  $radiob1 SetText "A radiobutton"
  $radiob1 SetValueAsInt 123

  set radiob1b [vtkKWRadioButton New]
  $radiob1b SetParent $parent
  $radiob1b Create
  $radiob1b SetText "Another radiobutton"
  $radiob1b SetValueAsInt 456

  $radiob1 SetSelectedState 1
  $radiob1b SetVariableName [$radiob1 GetVariableName] 

  pack [$radiob1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2
  pack [$radiob1b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create two radiobuttons. Use icons
  # They share the same variable name and each one has a different internal
  # value.

  set radiob2 [vtkKWRadioButton New]
  $radiob2 SetParent $parent
  $radiob2 Create
  $radiob2 SetImageToPredefinedIcon 100
  $radiob2 IndicatorVisibilityOff
  $radiob2 SetValue "foo"

  set radiob2b [vtkKWRadioButton New]
  $radiob2b SetParent $parent
  $radiob2b Create
  $radiob2b SetImageToPredefinedIcon 64
  $radiob2b IndicatorVisibilityOff
  $radiob2b SetValue "bar"

  $radiob2 SetSelectedState 1
  $radiob2b SetVariableName [$radiob2 GetVariableName] 

  pack [$radiob2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2
  pack [$radiob2b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create two radiobuttons. Use both labels and icons
  # They share the same variable name and each one has a different internal
  # value.

  set radiob3 [vtkKWRadioButton New]
  $radiob3 SetParent $parent
  $radiob3 Create
  $radiob3 SetText "Linear"
  $radiob3 SetImageToPredefinedIcon 40
  $radiob3 SetCompoundModeToLeft
  $radiob3 IndicatorVisibilityOff
  $radiob3 SetValue "foo"

  set radiob3b [vtkKWRadioButton New]
  $radiob3b SetParent $parent
  $radiob3b Create
  $radiob3b SetText "Log"
  $radiob3b SetImageToPredefinedIcon 41
  $radiob3b SetCompoundModeToLeft
  $radiob3b IndicatorVisibilityOff
  $radiob3b SetValue "bar"

  $radiob3 SetSelectedState 1
  $radiob3b SetVariableName [$radiob3 GetVariableName] 

  pack [$radiob3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2
  pack [$radiob3b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create a set of radiobutton
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  set radiob_set [vtkKWRadioButtonSet New]
  $radiob_set SetParent $parent
  $radiob_set Create
  $radiob_set SetBorderWidth 2
  $radiob_set SetReliefToGroove

  for {set id 0} {$id < 4} {incr id} {

    set radiob [$radiob_set AddWidget $id] 
    $radiob SetText "Radiobutton $id"
    $radiob SetBalloonHelpString \
      "This radiobutton is part of a unique set a vtkKWRadioButtonSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid. This classes automatically set the same variable name\
      among all radiobuttons as well as a unique value."
    }
  
  [$radiob_set GetWidget 0] SetSelectedState 1

  pack [$radiob_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # TODO: use vtkKWRadioButtonSetWithLabel and callbacks
  }

proc vtkKWRadioButtonGetType {} {
  return "TypeCore"
}
