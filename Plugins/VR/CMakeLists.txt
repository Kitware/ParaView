# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

cmake_minimum_required(VERSION 3.3)

# Find and Include ParaView
if(NOT ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif()

include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED
                QT4_COMPONENTS QtGui QtNetwork
                QT5_COMPONENTS Widgets Network)

# Checks to see if we want to use VRPN or VRUI.
# Both are disabled by default.
SET(PARAVIEW_USE_VRPN OFF CACHE BOOL "Build ParaView with VRPN support")
SET(PARAVIEW_USE_VRUI OFF CACHE BOOL "Build ParaView with VRUI support")

# if VRPN
mark_as_advanced(VRPN_LIBUSB_INCLUDE_DIR)
mark_as_advanced(VRPN_INCLUDE_DIR)
mark_as_advanced(VRPN_LIBRARY)
if(PARAVIEW_USE_VRPN)
  if(NOT EXISTS VRPN_INCLUDE_DIR)
    find_path(VRPN_INCLUDE_DIR vrpn_Tracker.h)
  endif()
  if(UNIX AND NOT APPLE)
    find_path(VRPN_LIBUSB_INCLUDE_DIR libusb.h PATHS /usr/include /usr/local/include)
    if(NOT EXISTS VRPN_LIBUSB_INCLUDE_DIR)
      # Not all platforms require this directory to be set
      set(VRPN_LIBUSB_INCLUDE_DIR "")
      mark_as_advanced(CLEAR VRPN_LIBUSB_INCLUDE_DIR)
    endif()
  endif()
  find_library(VRPN_LIBRARY vrpn)
  set(VRPN_MOC_SRC
    pqVRPNConnection.h
    pqVRPNEventListener.h
  )
  set(VRPN_SOURCE_FILES
    pqVRPNConnection.cxx
    pqVRPNEventListener.cxx
    vtkVRPNCallBackHandlers.cxx)
endif()

# if VRUI
if(PARAVIEW_USE_VRUI)
  set(VRUI_MOC_SRC pqVRUIConnection.h)
  set(VRUI_SOURCE_FILES
    pqVRUIConnection.cxx
    vtkVRUIPipe.cxx
    vtkVRUIServerState.cxx
    vtkVRUITrackerState.cxx)
endif()

pv_qt_wrap_cpp(MOC_SRCS
  pqVRAddConnectionDialog.h
  pqVRAddStyleDialog.h
  pqVRConnectionManager.h
  pqVRDockPanel.h
  pqVRQueueHandler.h
  pqVRStarter.h
  ${VRPN_MOC_SRC}
  ${VRUI_MOC_SRC}
  )

pv_qt_wrap_ui(UI_SRCS
  pqVRDockPanel.ui
  pqVRAddConnectionDialog.ui
  pqVRAddStyleDialog.ui
  )

if(PARAVIEW_USE_VRPN)
  INCLUDE_DIRECTORIES(${VRPN_INCLUDE_DIR} ${VRPN_LIBUSB_INCLUDE_DIR})
endif()

SET( VR_SOURCE_FILES
  pqVRConnectionManager.cxx
  pqVRQueueHandler.cxx
  ${VRPN_SOURCE_FILES}
  ${VRUI_SOURCE_FILES}
  vtkVRControlSliceOrientationStyle.cxx
  vtkVRControlSlicePositionStyle.cxx
  vtkVRGrabWorldStyle.cxx
  vtkVRInteractorStyle.cxx
  vtkVRInteractorStyleFactory.cxx
  vtkVRQueue.cxx
  vtkVRTrackStyle.cxx
  vtkVRSpaceNavigatorGrabWorldStyle.cxx
  vtkVRVirtualHandStyle.cxx
  ${MOC_SRCS})

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqVRStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

ADD_PARAVIEW_DOCK_WINDOW(
  IFACES_WINDOW IFACES_SRCS_WINDOW
  CLASS_NAME pqVRDockPanel
  DOCK_AREA Left)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(
  VRPlugin "1.0"
  GUI_INTERFACES ${IFACES} ${IFACES_WINDOW}
  GUI_SOURCES
          pqVRAddConnectionDialog.cxx
          pqVRAddStyleDialog.cxx
          pqVRDockPanel.cxx
          pqVRStarter.cxx
          ${MOC_SRCS}
          ${VR_SOURCE_FILES}
          ${RCS_SRCS} ${IFACE_SRCS} ${IFACES_SRCS_WINDOW}
          ${UI_SRCS}
          )

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVRConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVVRConfig.h
  ESCAPE_QUOTES IMMEDIATE)

# if VRPN is used then link to VRPN library
if(PARAVIEW_USE_VRPN)
  target_link_libraries (VRPlugin LINK_PRIVATE "${VRPN_LIBRARY}")
endif()

target_link_libraries (VRPlugin
  LINK_PRIVATE
    pqApplicationComponents
    vtkPVServerManagerRendering
    ${qt_targets}
)
