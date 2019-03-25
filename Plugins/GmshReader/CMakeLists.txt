list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

paraview_add_plugin(GmshReader
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES GmshReader::vtkGmshReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
