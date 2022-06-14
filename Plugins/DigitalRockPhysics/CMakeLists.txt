paraview_add_plugin(DigitalRockPhysics
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES DigitalRockPhysics::DigitalRocksFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
