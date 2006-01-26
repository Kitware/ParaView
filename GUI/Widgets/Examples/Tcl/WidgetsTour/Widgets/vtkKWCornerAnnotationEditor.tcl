proc vtkKWCornerAnnotationEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget
  # Set the corner annotation visibility

  set cae_renderwidget [vtkKWRenderWidget New]
  $cae_renderwidget SetParent $parent
  $cae_renderwidget Create
  $cae_renderwidget CornerAnnotationVisibilityOn

  pack [$cae_renderwidget GetWidgetName] -side right -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a volume reader

  set cae_reader [vtkXMLImageDataReader New]
  $cae_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "head100x100x47.vti"]

  # Create an image viewer
  # Use the render window and renderer of the renderwidget

  set cae_viewer [vtkImageViewer2 New]
  $cae_viewer SetRenderWindow [$cae_renderwidget GetRenderWindow] 
  $cae_viewer SetRenderer [$cae_renderwidget GetRenderer] 
  $cae_viewer SetInput [$cae_reader GetOutput] 
  $cae_viewer SetupInteractor [[$cae_renderwidget GetRenderWindow] GetInteractor]

  # Reset the window/level and the camera

  $cae_reader Update
  set range [[$cae_reader GetOutput] GetScalarRange]
  $cae_viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
  $cae_viewer SetColorLevel [expr 0.5 * ([lindex $range 1] + [lindex $range 0])]

  $cae_renderwidget ResetCamera

  # -----------------------------------------------------------------------

  # The corner annotation has the ability to parse "tags" and fill
  # them with information gathered from other objects.
  # For example let's display the slice and window/level in one corner
  # by connecting the corner annotation to our image actor and
  # image mapper

  set ca [$cae_renderwidget GetCornerAnnotation] 
  $ca SetImageActor [$cae_viewer GetImageActor] 
  $ca SetWindowLevel [$cae_viewer GetWindowLevel] 
  $ca SetText 2 "<slice>"
  $ca SetText 3 "<window>\n<level>"
  $ca SetText 1 "Hello World!"

  # -----------------------------------------------------------------------

  # Create a corner annotation editor
  # Connect it to the render widget
  
  set cae_anno_editor [vtkKWCornerAnnotationEditor New]
  $cae_anno_editor SetParent $parent
  $cae_anno_editor Create
  $cae_anno_editor SetRenderWidget $cae_renderwidget

  pack [$cae_anno_editor GetWidgetName] -side left -anchor nw -expand n -padx 2 -pady 2

  return "TypeVTK"
}
