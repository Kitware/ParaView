set(classes
  vtkExplicitStructuredGridGeneratorSource)
set(server_manager_files
  ExplicitStructuredGrid.xml)

_vtk_module_optional_dependency_exists(VTK::Python
  SATISFIED_VAR _have_vtk_python)
_vtk_module_optional_dependency_exists(VTK::WrappingPythonCore
  SATISFIED_VAR _have_vtk_wrappingpythoncore)
_vtk_module_optional_dependency_exists(VTK::PythonInterpreter
  SATISFIED_VAR _have_vtk_pythoninterpreter)

if (_have_vtk_python AND _have_vtk_wrappingpythoncore AND _have_vtk_pythoninterpreter)
  list(APPEND classes
    vtkExplicitStructuredGridPythonExtractor)
  list(APPEND server_manager_files
    ExplicitStructuredGridPythonExtractor.xml)
endif ()

vtk_module_add_module(ExplicitStructuredGrid::vtkExplicitStructuredGrid
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  ${server_manager_files})
