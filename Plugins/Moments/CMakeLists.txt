# This plugin contains a set of filters that helps analysis of flux and
# circulation fields.  Flux fields are defined on 2D cells and describe
# flows through the area of the cell.  Circulation fields are defined on 1D
# cells and describes flows in the direction of the cell.

vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  REQUEST_MODULES   Moments::MomentFilters
  PROVIDES_MODULES  moments_modules
  REQUIRES_MODULES  required_modules
  HIDE_MODULES_FROM_CACHE ON)

if (required_modules)
  foreach (required_module IN LISTS required_modules)
    if (NOT TARGET "${required_module}")
      message(FATAL_ERROR
        "Failed to find the required module ${required_module}.")
    endif ()
  endforeach ()
endif ()

vtk_module_build(
  MODULES             ${moments_modules}
  PACKAGE             Moments
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(Moments
  REQUIRED_ON_SERVER
  VERSION "0.0"
  MODULES Moments::MomentFilters)
