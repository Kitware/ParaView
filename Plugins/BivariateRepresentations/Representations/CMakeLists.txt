set(classes
  vtkBivariateNoiseMapper
  vtkBivariateNoiseRepresentation
  vtkBivariateTextureRepresentation
  vtkOpenGLBivariateNoiseMapperDelegator)

vtk_module_add_module(BivariateRepresentations::vtkBivariateRepresentations
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  BivariateRepresentations.xml)
