set(classes
  vtkPVZSpaceView)

vtk_module_add_module(zSpace::vtkZSpaceView
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS ZSpaceView.xml)
