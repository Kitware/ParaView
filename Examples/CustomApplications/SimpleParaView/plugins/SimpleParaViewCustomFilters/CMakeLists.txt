paraview_add_plugin(SimpleParaViewCustomFilters
  VERSION "1.0"
  MODULES ElevationFiltersApp
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ElevationFiltersApp/vtk.module")
