vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  REQUEST_MODULES   MooseXfemClip::vtkMooseXfemClip
  PROVIDES_MODULES  moose_modules
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
  MODULES             ${moose_modules}
  PACKAGE             MooseXfemClip
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(MooseXfemClip
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES MooseXfemClip::vtkMooseXfemClip
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
