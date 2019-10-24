paraview_add_plugin(Datamine
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES Datamine::DatamineReaders
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Readers/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
