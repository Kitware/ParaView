set(classes
  vtkBivariateNoiseMapper
  vtkBivariateNoiseRepresentation)

vtk_module_add_module(BivariateRepresentations::vtkBivariateRepresentations
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  BivariateRepresentations.xml)
