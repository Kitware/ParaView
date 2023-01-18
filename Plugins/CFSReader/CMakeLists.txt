paraview_add_plugin(CFSReader
  REQUIRED_ON_SERVER
  VERSION "22.04"
  MODULES CFSReader::Reader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
