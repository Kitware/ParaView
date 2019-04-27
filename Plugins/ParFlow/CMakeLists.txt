set (CMAKE_CXX_STANDARD 11)

paraview_add_plugin(
  ParFlow
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ParFlow::IO
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module"
)
