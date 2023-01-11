vtk_module_add_module(ContourLabelRepresentation::ContourLabelRepresentation
  CLASSES vtkContourLabelRepresentation)

paraview_add_server_manager_xmls(
  XMLS  ContourLabelPlugin.xml)
