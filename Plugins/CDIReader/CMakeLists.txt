paraview_add_plugin(CDIReader
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  MODULES CDIReader::vtkCDIReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
