set(classes
  vtkMySpecialPolyDataMapper
  vtkMySpecialRepresentation)

vtk_module_add_module(GeometryRepresentations
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Representation.xml)
