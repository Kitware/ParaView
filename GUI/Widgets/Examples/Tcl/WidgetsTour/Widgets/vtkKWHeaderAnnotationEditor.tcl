proc vtkKWHeaderAnnotationEditorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a render widget
  # Set the header annotation visibility and set some text

  vtkKWRenderWidget hae_renderwidget
  hae_renderwidget SetParent $parent
  hae_renderwidget Create $app

  hae_renderwidget HeaderAnnotationVisibilityOn
  hae_renderwidget SetHeaderAnnotationText "Hello World!"

  pack [hae_renderwidget GetWidgetName] -side right -fill both -expand y -padx 0 -pady 0

  # -----------------------------------------------------------------------

  # Create a volume reader

  vtkXMLImageDataReader hae_reader
  hae_reader SetFileName [file join [file dirname [info script]] ".." ".." Data "head100x100x47.vti"]

  # Create an image viewer
  # Use the render window and renderer of the renderwidget

  vtkImageViewer2 hae_viewer
  hae_viewer SetRenderWindow [hae_renderwidget GetRenderWindow] 
  hae_viewer SetRenderer [hae_renderwidget GetRenderer] 
  hae_viewer SetInput [hae_reader GetOutput] 

  vtkRenderWindowInteractor hae_iren
  hae_viewer SetupInteractor hae_iren

  hae_renderwidget ResetCamera

  # -----------------------------------------------------------------------

  # Create a header annotation editor
  # Connect it to the render widget
  
  vtkKWHeaderAnnotationEditor hae_anno_editor
  hae_anno_editor SetParent $parent
  hae_anno_editor Create $app
  hae_anno_editor SetRenderWidget hae_renderwidget

  pack [hae_anno_editor GetWidgetName] -side left -anchor nw -expand n -padx 2 -pady 2

  return "TypeVTK"
}

proc vtkKWHeaderAnnotationEditorFinalizePoint {} {
  hae_anno_editor Delete
  hae_reader Delete
  hae_iren Delete
  hae_renderwidget Delete
  hae_viewer Delete
}
