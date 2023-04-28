set(BUILD_SHARED_LIBS TRUE)

vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/LiveSourceDummy/vtk.module"
  REQUEST_MODULES   LiveSourceDummy
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(LiveSourceDummyFilter
  VERSION "1.0"
  MODULES LiveSourceDummy)
