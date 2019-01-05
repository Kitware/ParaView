vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Readers/vtk.module"
  REQUEST_MODULES   VDFReaderPlugin::vtkVaporReaders
  PROVIDES_MODULES  vapor_modules
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

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

vtk_module_build(
  MODULES             ${vapor_modules}
  PACKAGE             VDFReaderPlugin
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(VDFReaderPlugin
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES VDFReaderPlugin::vtkVaporReaders)
