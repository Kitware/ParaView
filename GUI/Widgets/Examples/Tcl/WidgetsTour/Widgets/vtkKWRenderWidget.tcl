proc vtkKWRenderWidgetEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget

  vtkKWRenderWidget rw_renderwidget
  rw_renderwidget SetParent $parent
  rw_renderwidget Create $app

  pack [rw_renderwidget GetWidgetName] -side top -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Switch to trackball style it's nicer

  [[[rw_renderwidget GetRenderWindow] GetInteractor] GetInteractorStyle] \
      SetCurrentStyleToTrackballCamera

  # Create a 3D object reader

  vtkXMLPolyDataReader rw_reader
  rw_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "teapot.vtp"]

  # Create the mapper and actor

  vtkPolyDataMapper rw_mapper
  rw_mapper SetInputConnection [rw_reader GetOutputPort] 

  vtkActor rw_actor
  rw_actor SetMapper rw_mapper

  # Add the actor to the scene

  rw_renderwidget AddProp rw_actor
  rw_renderwidget ResetCamera

  return "TypeVTK"
}

proc vtkKWRenderWidgetFinalizePoint {} {
  rw_reader Delete
  rw_actor Delete
  rw_mapper Delete
  rw_renderwidget Delete
}

