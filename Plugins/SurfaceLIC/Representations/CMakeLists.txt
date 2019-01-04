set(classes
  vtkSurfaceLICRepresentation)

vtk_module_add_module(SurfaceLIC::vtkSurfaceLICRepresentations
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  vtkSurfaceLICRepresentation.xml)
