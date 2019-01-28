vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/MyProxy/vtk.module"
  REQUEST_MODULES   MyProxy
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(SMMyProxy
  VERSION "1.0"
  MODULES MyProxy
  XML_DOCUMENTATION OFF)
