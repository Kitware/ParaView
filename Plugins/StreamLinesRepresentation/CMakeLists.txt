set(sources)
set(interfaces)

if (PARAVIEW_USE_QT)
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
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF)

if (PARAVIEW_USE_QT)
  target_link_libraries(StreamLinesRepresentation
    PRIVATE
      ParaView::RemotingViews)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
