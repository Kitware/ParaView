proc vtkKWListBoxToListBoxSelectionEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a list box to list box selection editor

  set lb2lb1 [vtkKWListBoxToListBoxSelectionEditor New]
  $lb2lb1 SetParent $parent
  $lb2lb1 Create
  $lb2lb1 SetReliefToGroove
  $lb2lb1 SetBorderWidth 2
  $lb2lb1 SetPadX 2
  $lb2lb1 SetPadY 2

  $lb2lb1 AddSourceElement "Monday" 0
  $lb2lb1 AddSourceElement "Tuesday" 0
  $lb2lb1 AddSourceElement "Wednesday" 0
  $lb2lb1 AddFinalElement "Thursday" 0
  $lb2lb1 AddFinalElement "Friday" 0

  pack [$lb2lb1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  return "TypeComposite"
}
