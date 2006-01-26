proc vtkKWRenderWidgetEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget

  set rw_renderwidget [vtkKWRenderWidget New]
  $rw_renderwidget SetParent $parent
  $rw_renderwidget Create

  pack [$rw_renderwidget GetWidgetName] -side top -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a 3D object reader

  set rw_reader [vtkXMLPolyDataReader New]
  $rw_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "teapot.vtp"]

  # Create the mapper and actor

  set rw_mapper [vtkPolyDataMapper New]
  $rw_mapper SetInputConnection [$rw_reader GetOutputPort] 

  set rw_actor [vtkActor New]
  $rw_actor SetMapper $rw_mapper

  # Add the actor to the scene

  $rw_renderwidget AddViewProp $rw_actor
  $rw_renderwidget ResetCamera

  return "TypeVTK"
}
