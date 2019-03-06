find_package(Qt5 QUIET REQUIRED COMPONENTS Network)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PARAVIEW_PLUGIN_VRPlugin_USE_VRPN "Build VRPlugin with VRPN support" OFF)
mark_as_advanced(PARAVIEW_PLUGIN_VRPlugin_USE_VRPN)
option(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI "Build VRPlugin with VRUI support" OFF)
mark_as_advanced(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI)

# TODO: There's a QTSOCK preprocessor definition floating around. What should
# be done with that?

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
  vtkVRControlSliceOrientationStyle.cxx
  vtkVRControlSliceOrientationStyle.h
  vtkVRControlSlicePositionStyle.cxx
  vtkVRControlSlicePositionStyle.h
  vtkVRGrabWorldStyle.cxx
  vtkVRGrabWorldStyle.h
  vtkVRInteractorStyle.cxx
  vtkVRInteractorStyle.h
  vtkVRInteractorStyleFactory.cxx
  vtkVRInteractorStyleFactory.h
  vtkVRQueue.cxx
  vtkVRQueue.h
  vtkVRSpaceNavigatorGrabWorldStyle.cxx
  vtkVRSpaceNavigatorGrabWorldStyle.h
  vtkVRTrackStyle.cxx
  vtkVRTrackStyle.h
  vtkVRVirtualHandStyle.cxx
  vtkVRVirtualHandStyle.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVRConfig.h")


if (PARAVIEW_PLUGIN_VRPlugin_USE_VRPN)
  find_package(VRPN REQUIRED)
  list(APPEND sources
    pqVRPNConnection.cxx
    pqVRPNConnection.h
    pqVRPNEventListener.cxx
    pqVRPNEventListener.h
    vtkVRPNCallBackHandlers.cxx
    vtkVRPNCallBackHandlers.h)
endif ()

if (PARAVIEW_PLUGIN_VRPlugin_USE_VRUI)
  set(sources
    pqVRUIConnection.cxx
    pqVRUIConnection.h
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

paraview_add_plugin(VRPlugin
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_FILES ${ui_files}
  SOURCES ${sources})

target_include_directories(VRPlugin
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(VRPlugin
  PRIVATE
    ParaView::ClientServerCoreCore
    ParaView::Core
    ParaView::ServerManagerCore
    ParaView::ServerManagerRendering
    ParaView::pqApplicationComponents
    VTK::CommonCore
    VTK::CommonMath
    VTK::CommonTransforms
    VTK::RenderingCore)

if (PARAVIEW_PLUGIN_VRPlugin_USE_VRPN)
  target_link_libraries(VRPlugin
    PRIVATE
      VRPN::VRPN)
endif ()
