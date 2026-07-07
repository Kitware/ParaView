paraview_add_plugin(DPvtkReader
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES DPvtkReader::vtkDPvtkReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
