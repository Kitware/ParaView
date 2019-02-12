vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/BagPlot/vtk.module"
  REQUEST_MODULES   BagPlotViewsAndFilters::BagPlotViewsAndFiltersBagPlot
  PROVIDES_MODULES  bagplot_modules
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
  MODULES             ${bagplot_modules}
  PACKAGE             BagPlotViewsAndFilters
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(qrc_files)

if (PARAVIEW_BUILD_QT_GUI)
  list(APPEND qrc_files
    BagPlotViewsAndFilters.qrc)
endif ()

paraview_add_plugin(BagPlotViewsAndFilters
  REQUIRED_ON_SERVER
  VERSION "1.0"
  SERVER_MANAGER_XML BagPlotViewsAndFilters.xml
  UI_RESOURCES ${qrc_files}
  MODULES BagPlotViewsAndFilters::BagPlotViewsAndFiltersBagPlot)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
