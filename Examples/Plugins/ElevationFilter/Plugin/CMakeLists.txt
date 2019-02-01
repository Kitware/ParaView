vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/ElevationFilters/vtk.module"
  REQUEST_MODULES   ElevationFilters
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(ElevationFilter
  VERSION "1.0"
  MODULES ElevationFilters)
