set(classes
  vtkPVPanoramicProjectionView)

vtk_module_add_module(PanoramicProjectionView::vtkPanoramicProjectionViews
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  PanoramicProjectionView.xml)
