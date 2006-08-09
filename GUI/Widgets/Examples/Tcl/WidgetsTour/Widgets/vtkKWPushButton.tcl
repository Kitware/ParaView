proc vtkKWPushButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a push button

  set pushbutton1 [vtkKWPushButton New]
  $pushbutton1 SetParent $parent
  $pushbutton1 Create
  $pushbutton1 SetText "A push button"

  pack [$pushbutton1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another push button, use an icon

  set pushbutton2 [vtkKWPushButton New]
  $pushbutton2 SetParent $parent
  $pushbutton2 Create
  $pushbutton2 SetImageToPredefinedIcon 1
  $pushbutton2 SetBalloonHelpString \
    "Another pushbutton using one of the predefined icons"

  pack [$pushbutton2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another push button, use both text and icon

  set pushbutton2b [vtkKWPushButton New]
  $pushbutton2b SetParent $parent
  $pushbutton2b Create
  $pushbutton2b SetText "A push button with an icon"
  $pushbutton2b SetImageToPredefinedIcon 171
  $pushbutton2b SetCompoundModeToLeft
  $pushbutton2b SetBalloonHelpString \
    "Another pushbutton, using both a text and one of the predefined icons"

  pack [$pushbutton2b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another push button with a label this time

  set pushbutton3 [vtkKWPushButtonWithLabel New]
  $pushbutton3 SetParent $parent
  $pushbutton3 Create
  $pushbutton3 SetLabelText "Press this..."
  [$pushbutton3 GetWidget] SetText "button"
  $pushbutton3 SetBalloonHelpString \
    "This is a vtkKWPushButtonWithLabel i.e. a pushbutton associated to a\
    label that can be positioned around the pushbutton."

  pack [$pushbutton3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another push button, with a menu

  set pushbutton4 [vtkKWPushButtonWithMenu New]
  $pushbutton4 SetParent $parent
  $pushbutton4 Create
  [$pushbutton4 GetPushButton] SetImageToPredefinedIcon 128
  $pushbutton4 SetBalloonHelpString \
      "This is a vtkKWPushButtonWithMenu, i.e. a pushbutton associated to a\
      menu."

  set menu [$pushbutton4 GetMenu]
  $menu AddCommand "Microsoft Office"
  $menu AddCommand "Program Files"
  $menu AddCommand "C:"

  pack [$pushbutton4 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a set of pushbutton
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  set pushbutton_set [vtkKWPushButtonSet New]
  $pushbutton_set SetParent $parent
  $pushbutton_set Create
  $pushbutton_set SetBorderWidth 2
  $pushbutton_set SetReliefToGroove
  $pushbutton_set SetWidgetsPadX 1
  $pushbutton_set SetWidgetsPadY 1
  $pushbutton_set SetPadX 1
  $pushbutton_set SetPadY 1
  $pushbutton_set ExpandWidgetsOn
  $pushbutton_set SetMaximumNumberOfWidgetsInPackingDirection 3

  set math [vtkMath New]

  for {set id 0} {$id < 9} {incr id} {

    set pushbutton [$pushbutton_set AddWidget $id] 
    $pushbutton SetText "Push button $id"
    eval $pushbutton SetBackgroundColor \
      [$math HSVToRGB [expr $id / 8.0] 0.3 0.75]
    $pushbutton SetBalloonHelpString \
      "This pushbutton is part of a unique set a vtkKWPushButtonSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid. Each button is assigned a different color."
  }

  [$pushbutton_set GetWidget 0] SetText "I'm the first button"

  pack [$pushbutton_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # TODO: add callbacks
}

proc vtkKWPushButtonGetType {} {
  return "TypeCore"
}
