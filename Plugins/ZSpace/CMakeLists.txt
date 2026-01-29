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

set (xml_files)
if (PARAVIEW_USE_PYTHON)
  list (APPEND xml_files
    "${CMAKE_CURRENT_SOURCE_DIR}/zSpace/vtkPVZSpaceSettings.xml")
endif ()

paraview_add_plugin(zSpace
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES zSpace::vtkZSpaceView
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/zSpace/vtk.module"
  SERVER_MANAGER_XML ${xml_files}
  XML_DOCUMENTATION OFF)

if (PARAVIEW_USE_QT)
  set (private_lib_dependencies
    ParaView::pqApplicationComponents
    ParaView::RemotingViews)

  if (PARAVIEW_USE_PYTHON)
    list (APPEND private_lib_dependencies ParaView::pqPython)
  endif ()

  target_link_libraries(zSpace PRIVATE ${private_lib_dependencies})
  target_compile_definitions(zSpace PRIVATE
    QT_NO_KEYWORD
    PARAVIEW_USE_PYTHON=$<BOOL:${PARAVIEW_USE_PYTHON}>)
endif ()
