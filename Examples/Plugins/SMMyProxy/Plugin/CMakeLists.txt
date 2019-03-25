paraview_add_plugin(SMMyProxy
  VERSION "1.0"
  MODULES MyProxy
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/MyProxy/vtk.module"
  XML_DOCUMENTATION OFF)
