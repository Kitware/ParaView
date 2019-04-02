vtk_module_scan(
  MODULE_FILES            "${CMAKE_CURRENT_SOURCE_DIR}/Filter/vtk.module"
  REQUEST_MODULES         ExplicitStructuredGrid::vtkExplicitStructuredGrid
  PROVIDES_MODULES        explicitstructuredgrid_modules
  REQUIRES_MODULES        required_modules
  HIDE_MODULES_FROM_CACHE ON
)

if(required_modules)
  foreach(module IN LISTS required_modules)
    if(NOT TARGET "${module}")
      message(FATAL_ERROR
        "Failed to find the required module ${module}."
      )
    endif()
  endforeach()
endif()

vtk_module_build(
  MODULES             ${explicitstructuredgrid_modules}
  PACKAGE             ExplicitStructuredGrid
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1"
)

set(SM_XML_FILES ExplicitStructuredGrid.xml)

if(PARAVIEW_ENABLE_PYTHON)
  list(APPEND SM_XML_FILES ExplicitStructuredGridPythonExtractor.xml)
endif()

paraview_add_plugin(ExplicitStructuredGrid
  REQUIRED_ON_SERVER
  VERSION "1.0"
  SERVER_MANAGER_XML ${SM_XML_FILES}
  MODULES ExplicitStructuredGrid::vtkExplicitStructuredGrid
)

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
