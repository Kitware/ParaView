set(classes
  vtkSlowFilter)

vtk_module_add_module(SlowFilter::SlowFilter
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  vtkSlowFilter.xml)
