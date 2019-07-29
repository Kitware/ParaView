set(classes
  vtkExplicitStructuredGridGeneratorSource)
set(server_manager_files
  ExplicitStructuredGrid.xml)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND classes
    vtkExplicitStructuredGridPythonExtractor)
  list(APPEND server_manager_files
    ExplicitStructuredGridPythonExtractor.xml)
endif ()

vtk_module_add_module(ExplicitStructuredGrid::vtkExplicitStructuredGrid
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  ${server_manager_files})
