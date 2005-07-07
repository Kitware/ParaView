proc vtkKWCornerAnnotationEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget
  # Set the corner annotation visibility

  vtkKWRenderWidget RenderWidget
  RenderWidget SetParent $parent
  RenderWidget Create $app
  RenderWidget CornerAnnotationVisibilityOn

  pack [RenderWidget GetWidgetName] -side right -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a volume reader

  vtkXMLImageDataReader Reader
  Reader SetFileName [file join [file dirname [info script]] ".." ".." Data "head100x100x47.vti"]

  # Create an image viewer
  # Use the render window and renderer of the renderwidget

  vtkImageViewer2 Viewer
  Viewer SetRenderWindow [RenderWidget GetRenderWindow] 
  Viewer SetRenderer [RenderWidget GetRenderer] 
  Viewer SetInput [Reader GetOutput] 

  vtkRenderWindowInteractor Interactor
  Viewer SetupInteractor Interactor

  # Reset the window/level and the camera

  Reader Update
  set range [[Reader GetOutput] GetScalarRange]
  Viewer SetColorWindow [expr [lindex $range 1] - [lindex $range 0]]
  Viewer SetColorLevel [expr 0.5 *  [lindex $range 1] + [lindex $range 0]]

  RenderWidget ResetCamera

  # -----------------------------------------------------------------------

  # The corner annotation has the ability to parse "tags" and fill
  # them with information gathered from other objects.
  # For example let's display the slice and window/level in one corner
  # by connecting the corner annotation to our image actor and
  # image mapper

  set ca [RenderWidget GetCornerAnnotation] 
  $ca SetImageActor [Viewer GetImageActor] 
  $ca SetWindowLevel [Viewer GetWindowLevel] 
  $ca SetText 2 "<slice>"
  $ca SetText 3 "<window>\n<level>"
  $ca SetText 1 "Hello World!"

  # -----------------------------------------------------------------------

  # Create a corner annotation editor
  # Connect it to the render widget
  
  vtkKWCornerAnnotationEditor CornerAnnotationEditor
  CornerAnnotationEditor SetParent $parent
  CornerAnnotationEditor Create $app
  CornerAnnotationEditor SetRenderWidget RenderWidget

  pack [CornerAnnotationEditor GetWidgetName] -side left -anchor nw -expand n -padx 2 -pady 2

  return "TypeVTK"
}

proc vtkKWCornerAnnotationEditorFinalizePoint {} {
  CornerAnnotationEditor Delete
  Reader Delete
  Interactor Delete
  RenderWidget Delete
  Viewer Delete
}
