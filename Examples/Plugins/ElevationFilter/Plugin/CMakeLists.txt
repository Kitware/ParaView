paraview_add_plugin(ElevationFilter
  VERSION "1.0"
  MODULES ElevationFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ElevationFilters/vtk.module")

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
