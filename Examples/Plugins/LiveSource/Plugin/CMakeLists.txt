set(BUILD_SHARED_LIBS TRUE)

paraview_add_plugin(LiveSourceDummy
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES LiveSourceDummySources
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LiveSourceDummySources/vtk.module")

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
