paraview_add_plugin(GeographicalMap
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES GeographicalMap::vtkGeographicalMap
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")
