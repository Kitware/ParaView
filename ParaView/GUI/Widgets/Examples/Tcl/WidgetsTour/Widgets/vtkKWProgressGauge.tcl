proc vtkKWProgressGaugeEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a progress gauge

  vtkKWProgressGauge progress1
  progress1 SetParent $parent
  progress1 Create
  progress1 SetWidth 150
  progress1 SetBorderWidth 2
  progress1 SetReliefToGroove
  progress1 SetPadX 2
  progress1 SetPadY 2

  pack [progress1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create a set of pushbutton that will modify the progress gauge

  vtkKWPushButtonSet progress1_pbs
  progress1_pbs SetParent $parent
  progress1_pbs Create
  progress1_pbs SetBorderWidth 2
  progress1_pbs SetReliefToGroove
  progress1_pbs SetWidgetsPadX 1
  progress1_pbs SetWidgetsPadY 1
  progress1_pbs SetPadX 1
  progress1_pbs SetPadY 1
  progress1_pbs ExpandWidgetsOn

  for {set id 0} {$id <= 100} {incr id 25} {
    set pushbutton [progress1_pbs AddWidget $id] 
    $pushbutton SetText "Set Progress to $id%"
    $pushbutton SetCommand progress1 "SetValue $id"
  }

  # Add a special button that will iterate from 0 to 100% in Tcl

  set pushbutton [progress1_pbs AddWidget 1000] 
  $pushbutton SetText "0% to 100%"
  $pushbutton SetCommand "" {
    for {set i 0} {$i <= 100} {incr i} {
      progress1 SetValue $i; after 20; update
    }
  }
  
  pack [progress1_pbs GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # TODO: add callbacks

  return "TypeComposite"
}

proc vtkKWProgressGaugeFinalizePoint {} {
  progress1 Delete
  progress1_pbs Delete
}
