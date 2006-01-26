proc vtkKWLoadSaveButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a load button

  set load_button1 [vtkKWLoadSaveButton New]
  $load_button1 SetParent $parent
  $load_button1 Create
  $load_button1 SetText "Click to Pick a File"
  [$load_button1 GetLoadSaveDialog] SaveDialogOff

  pack [$load_button1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  return "TypeComposite"
}
