proc vtkKWNotebookEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a notebook

  set notebook1 [vtkKWNotebook New]
  $notebook1 SetParent $parent
  $notebook1 SetMinimumWidth 400
  $notebook1 SetMinimumHeight 200
  $notebook1 Create

  pack [$notebook1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # Add some pages

  $notebook1 AddPage "Page 1"

  $notebook1 AddPage "Page Blue"
  [$notebook1 GetFrame "Page Blue"] SetBackgroundColor 0.2 0.2 0.9

  set page_id [$notebook1 AddPage "Page Red"] 
  [$notebook1 GetFrame $page_id] SetBackgroundColor 0.9 0.2 0.2

  return "TypeComposite"
}
