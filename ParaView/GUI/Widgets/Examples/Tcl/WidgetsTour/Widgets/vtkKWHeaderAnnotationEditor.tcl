proc vtkKWHeaderAnnotationEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget
  # Set the header annotation visibility and set some text

  set hae_renderwidget [vtkKWRenderWidget New]
  $hae_renderwidget SetParent $parent
  $hae_renderwidget Create

  $hae_renderwidget HeaderAnnotationVisibilityOn
  $hae_renderwidget SetHeaderAnnotationText "Hello World!"

  pack [$hae_renderwidget GetWidgetName] -side right -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a volume reader

  set hae_reader [vtkXMLImageDataReader New]
  $hae_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "head100x100x47.vti"]

  # Create an image viewer
  # Use the render window and renderer of the renderwidget

  set hae_viewer [vtkImageViewer2 New]
  $hae_viewer SetRenderWindow [$hae_renderwidget GetRenderWindow] 
  $hae_viewer SetRenderer [$hae_renderwidget GetRenderer] 
  $hae_viewer SetInput [$hae_reader GetOutput] 
  $hae_viewer SetupInteractor [[$hae_renderwidget GetRenderWindow] GetInteractor]

  $hae_renderwidget ResetCamera

  # -----------------------------------------------------------------------

  # Create a header annotation editor
  # Connect it to the render widget
  
  set hae_anno_editor [vtkKWHeaderAnnotationEditor New]
  $hae_anno_editor SetParent $parent
  $hae_anno_editor Create
  $hae_anno_editor SetRenderWidget $hae_renderwidget

  pack [$hae_anno_editor GetWidgetName] -side left -anchor nw -expand n -padx 2 -pady 2

  return "TypeVTK"
}
