set(classes
  vtkSMMyElevationProxy
  vtkSMMyElevationProxy)

vtk_module_add_module(MyProxy
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MyProxy.xml)
