set(classes
  vtkMyElevationFilter)

vtk_module_add_module(ElevationFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MyElevationFilter.xml)
