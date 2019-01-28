vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/GeometryRepresentations/vtk.module"
  REQUEST_MODULES   GeometryRepresentations
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(Representation
  VERSION "1.0"
  MODULES GeometryRepresentations
  XML_DOCUMENTATION OFF)
