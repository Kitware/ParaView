set(classes
  vtkPVStereoCursorView)

vtk_module_add_module(StereoCursorView::StereoCursorViews
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS StereoCursorView.xml)
