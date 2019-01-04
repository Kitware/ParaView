vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  REQUEST_MODULES   StreamLinesRepresentation::vtkStreamLines
  PROVIDES_MODULES  streamlines_modules
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
  MODULES             ${streamlines_modules}
  PACKAGE             StreamLinesRepresentation
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(sources)
set(interfaces)

if (PARAVIEW_BUILD_QT_GUI)
  list(APPEND sources
    pqStreamLinesAnimationManager.cxx
    pqStreamLinesAnimationManager.h)

  paraview_plugin_add_auto_start(
    CLASS_NAME "pqStreamLinesAnimationManager"
    STARTUP onStartup
    SHUTDOWN onShutdown
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})
endif ()

paraview_add_plugin(StreamLinesRepresentation
  VERSION "0.1"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES StreamLinesRepresentation::vtkStreamLines
  XML_DOCUMENTATION OFF)

if (PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(StreamLinesRepresentation
    PRIVATE
      ParaView::ServerManagerCore
      ParaView::ServerManagerRendering)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
