vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  REQUEST_MODULES   SurfaceLIC::vtkSurfaceLICRepresentations
  PROVIDES_MODULES  surfacelic_modules
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
  MODULES             ${surfacelic_modules}
  PACKAGE             SurfaceLIC
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(SurfaceLIC
  VERSION "2.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  MODULES SurfaceLIC::vtkSurfaceLICRepresentations)
