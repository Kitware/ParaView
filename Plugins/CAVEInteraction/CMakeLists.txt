find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" QUIET REQUIRED COMPONENTS Network)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN "Build CAVEInteraction plugin with VRPN support" OFF)
mark_as_advanced(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN)
option(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI "Build CAVEInteraction plugin with VRUI support" OFF)
mark_as_advanced(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI)

# TODO: Should something be done with the files in samples/?

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVRConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVRConfig.h"
  @ONLY)

set(sources
  pqVRAddConnectionDialog.cxx
  pqVRAddConnectionDialog.h
  pqVRAddStyleDialog.cxx
  pqVRAddStyleDialog.h
  pqVRConnectionManager.cxx
  pqVRConnectionManager.h
  pqVRDockPanel.cxx
  pqVRDockPanel.h
  pqVRQueueHandler.cxx
  pqVRQueueHandler.h
  pqVRStarter.cxx
  pqVRStarter.h
  vtkVRInteractorStyleFactory.cxx
  vtkVRInteractorStyleFactory.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVRConfig.h")


if (PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN)
  find_package(VRPN REQUIRED)
  list(APPEND sources
    pqVRPNConnection.cxx
    pqVRPNConnection.h
    pqVRPNEventListener.cxx
    pqVRPNEventListener.h
    vtkVRPNCallBackHandlers.cxx
    vtkVRPNCallBackHandlers.h)
endif ()

if (PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI)
  list(APPEND sources
    pqVRUIConnection.cxx
    pqVRUIConnection.h
    vtkVRUIPipe.cxx
    vtkVRUIPipe.h
    vtkVRUIServerState.cxx
    vtkVRUIServerState.h
    vtkVRUITrackerState.cxx
    vtkVRUITrackerState.h)
endif ()

set(ui_files
  pqVRAddConnectionDialog.ui
  pqVRAddStyleDialog.ui
  pqVRDockPanel.ui)

set(interfaces)
paraview_plugin_add_auto_start(
  CLASS_NAME "pqVRStarter"
  STARTUP onStartup
  SHUTDOWN onShutdown
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_plugin_add_dock_window(
  CLASS_NAME pqVRDockPanel
  DOCK_AREA Left
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

set(xml_files
  CAVEInteraction_SM.xml)
if (PARAVIEW_USE_PYTHON)
  list(APPEND xml_files
    CAVEInteraction_SM_Python.xml)
endif ()

paraview_add_plugin(CAVEInteraction
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_FILES ${ui_files}
  SOURCES ${sources}
  SERVER_MANAGER_XML ${xml_files}
  MODULES ParaViewIncubator::CAVEInteractionStyles
  XML_DOCUMENTATION OFF)

target_include_directories(CAVEInteraction
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(CAVEInteraction
  PRIVATE
    ParaView::RemotingCore
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    ParaView::pqApplicationComponents
    VTK::CommonCore
    VTK::CommonMath
    VTK::CommonTransforms
    VTK::RenderingCore
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Network")
target_compile_definitions(CAVEInteraction PRIVATE QT_NO_KEYWORDS)

if (PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN)
  target_link_libraries(CAVEInteraction
    PRIVATE
      VRPN::VRPN)
endif ()
