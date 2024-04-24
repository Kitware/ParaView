set(classes
  vtkUnstructuredGridGhostCellsGenerator)

vtk_module_add_module(LegacyGhostCellsGeneratorFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  LegacyGhostCellsGenerator.xml)
