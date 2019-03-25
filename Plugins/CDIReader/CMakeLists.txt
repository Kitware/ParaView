paraview_add_plugin(CDIReader
  REQUIRED_ON_SERVER
  VERSION "1.3"
  MODULES CDIReader::vtkCDIReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")
