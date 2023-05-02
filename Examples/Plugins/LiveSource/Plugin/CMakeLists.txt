set(BUILD_SHARED_LIBS TRUE)

vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/LiveSourceDummySources/vtk.module"
  REQUEST_MODULES   LiveSourceDummySources
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(LiveSourceDummy
  VERSION "1.0"
  MODULES LiveSourceDummySources)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
