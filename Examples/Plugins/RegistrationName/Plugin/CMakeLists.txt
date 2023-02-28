paraview_add_plugin(RegistrationName
  VERSION "1.0"
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  MODULES MyReader ElevationFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/MyReader/vtk.module" "${CMAKE_CURRENT_SOURCE_DIR}/ElevationFilters/vtk.module"
  SERVER_MANAGER_XML RegistrationName.xml)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
