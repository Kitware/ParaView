proc vtkKWCornerAnnotationEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget
  # Set the corner annotation visibility

  vtkKWRenderWidget cae_renderwidget
  cae_renderwidget SetParent $parent
  cae_renderwidget Create $app
  cae_renderwidget CornerAnnotationVisibilityOn

  pack [cae_renderwidget GetWidgetName] -side right -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a volume reader

  vtkXMLImageDataReader cae_reader
  cae_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "head100x100x47.vti"]

  # Create an image viewer
  # Use the render window and renderer of the renderwidget

  vtkImageViewer2 cae_viewer
  cae_viewer SetRenderWindow [cae_renderwidget GetRenderWindow] 
  cae_viewer SetRenderer [cae_renderwidget GetRenderer] 
  cae_viewer SetInput [cae_reader GetOutput] 
  cae_viewer SetupInteractor [[cae_renderwidget GetRenderWindow] GetInteractor]

  # Reset the window/level and the camera

  cae_reader Update
  set range [[cae_reader GetOutput] GetScalarRange]
  cae_viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
  cae_viewer SetColorLevel [expr 0.5 * ([lindex $range 1] + [lindex $range 0])]

  cae_renderwidget ResetCamera

  # -----------------------------------------------------------------------

  # The corner annotation has the ability to parse "tags" and fill
  # them with information gathered from other objects.
  # For example let's display the slice and window/level in one corner
  # by connecting the corner annotation to our image actor and
  # image mapper

  set ca [cae_renderwidget GetCornerAnnotation] 
  $ca SetImageActor [cae_viewer GetImageActor] 
  $ca SetWindowLevel [cae_viewer GetWindowLevel] 
  $ca SetText 2 "<slice>"
  $ca SetText 3 "<window>\n<level>"
  $ca SetText 1 "Hello World!"

  # -----------------------------------------------------------------------

  # Create a corner annotation editor
  # Connect it to the render widget
  
  vtkKWCornerAnnotationEditor cae_anno_editor
  cae_anno_editor SetParent $parent
  cae_anno_editor Create $app
  cae_anno_editor SetRenderWidget cae_renderwidget

  pack [cae_anno_editor GetWidgetName] -side left -anchor nw -expand n -padx 2 -pady 2

  return "TypeVTK"
}

proc vtkKWCornerAnnotationEditorFinalizePoint {} {
  cae_anno_editor Delete
  cae_reader Delete
  cae_renderwidget Delete
  cae_viewer Delete
}
