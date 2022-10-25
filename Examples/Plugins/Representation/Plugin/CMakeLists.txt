paraview_add_plugin(Representation
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES GeometryRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/GeometryRepresentations/vtk.module"
  XML_DOCUMENTATION OFF)
