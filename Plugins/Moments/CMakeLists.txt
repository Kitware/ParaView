# This plugin contains a set of filters that helps analysis of flux and
# circulation fields.  Flux fields are defined on 2D cells and describe
# flows through the area of the cell.  Circulation fields are defined on 1D
# cells and describes flows in the direction of the cell.

paraview_add_plugin(Moments
  REQUIRED_ON_SERVER
  VERSION "0.0"
  MODULES Moments::MomentFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")
