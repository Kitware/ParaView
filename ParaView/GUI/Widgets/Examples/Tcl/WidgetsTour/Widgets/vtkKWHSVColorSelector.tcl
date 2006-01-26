proc vtkKWHSVColorSelectorEntryPoint {parent win} {

  set app [$parent GetApplication]

  # Create a color selector

  set ccb [vtkKWHSVColorSelector New]
  $ccb SetParent $parent
  $ccb Create
  $ccb SetSelectionChangingCommand $parent "SetBackgroundColor"
  $ccb InvokeCommandsWithRGBOn
  $ccb SetBalloonHelpString "This HSV Color Selector changes the background color of its parent"

  set math [vtkMath New]
  eval $ccb SetSelectedColor [eval $math RGBToHSV [$parent GetBackgroundColor]]
  
  pack [$ccb GetWidgetName] -side top -anchor nw -expand y -padx 2 -pady 2

  return "TypeComposite"
}
