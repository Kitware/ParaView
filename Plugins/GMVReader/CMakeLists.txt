vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module"
  REQUEST_MODULES   GMVReader::vtkGMVReader
  PROVIDES_MODULES  gmvreader_modules
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
  MODULES             ${gmvreader_modules}
  PACKAGE             GMVReader
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(GMVReader
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES GMVReader::vtkGMVReader)

# TODO: What to do with pqGMVReaderPanel? This is needed if it is to be used.
target_link_libraries(GMVReader
  PRIVATE
    ParaView::Core
    ParaView::ServerManagerCore)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
