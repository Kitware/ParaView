paraview_add_plugin(GeodesicMeasurement
  VERSION "1.0"
  MODULES GeodesicMeasurement::GeodesicMeasurementFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")
