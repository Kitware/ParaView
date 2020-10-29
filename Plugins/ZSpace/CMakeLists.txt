list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(sources)
set(interfaces)

if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqZSpaceManager.cxx
    pqZSpaceManager.h)

  paraview_plugin_add_auto_start(
    CLASS_NAME "pqZSpaceManager"
    STARTUP onStartup
    SHUTDOWN onShutdown
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})
endif ()

paraview_add_plugin(zSpace
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES zSpace::vtkZSpaceView
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/zSpace/vtk.module"
  XML_DOCUMENTATION OFF)

if (PARAVIEW_USE_QT)
  target_link_libraries(zSpace
    PRIVATE
      ParaView::RemotingViews)
endif ()
