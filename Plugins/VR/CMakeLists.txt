# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# ParaView
if(NOT ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
endif()

# Optionally check if we want to use VRPN too (default is VRUI)
SET(PARAVIEW_USE_VRPN OFF CACHE BOOL "Build ParaView with VRPN support. VRUI is default")

if(PARAVIEW_USE_VRPN)
  if(NOT EXISTS VRPN_INCLUDE_DIR)
    find_path(VRPN_INCLUDE_DIR vrpn_Tracker.h)
  endif()

  find_library(VRPN_LIBRARY vrpn)
  set(VRPN_MOC_SRC vtkVRPNConnection.h)
  set(VRPN_SOURCE_FILES
    vtkVRPNConnection.cxx
    vtkVRPNCallBackHandlers.cxx)
else(PARAVIEW_USE_VRPN)
  mark_as_advanced(VRPN_INCLUDE_DIR)
  mark_as_advanced(VRPN_LIBRARY)
endif(PARAVIEW_USE_VRPN)

QT4_WRAP_CPP(MOC_SRCS
  pqVRStarter.h
  vtkVRConnectionManager.h
  ${VRPN_MOC_SRC}
  vtkVRUIConnection.h
  vtkVRQueue.h
  vtkVRQueueHandler.h
  vtkVRInteractorStyle.h
  vtkVRStyleTracking.h
  vtkVRStyleGrabNUpdateMatrix.h
  vtkVRStyleGrabNRotateSliceNormal.h
  vtkVRStyleGrabNTranslateSliceOrigin.h
  vtkVRActiveObjectManipulationStyle.h
  )

SET (QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  ${pqCore_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqComponents_BINARY_DIR}
  )

if(PARAVIEW_USE_VRPN)
  INCLUDE_DIRECTORIES(${VRPN_INCLUDE_DIR})
endif(PARAVIEW_USE_VRPN)

SET( VR_SOURCE_FILES
  vtkVRConnectionManager.cxx
  ${VRPN_SOURCE_FILES}
  vtkVRUIConnection.cxx
  vtkVRUIPipe.cxx
  vtkVRUIServerState.cxx
  vtkVRUITrackerState.cxx
  vtkVRQueue.cxx
  vtkVRQueueHandler.cxx
  vtkVRInteractorStyle.cxx
  vtkVRStyleTracking.cxx
  vtkVRStyleGrabNUpdateMatrix.cxx
  vtkVRStyleGrabNRotateSliceNormal.cxx
  vtkVRStyleGrabNTranslateSliceOrigin.cxx
  vtkVRActiveObjectManipulationStyle.cxx
  ${MOC_SRCS})

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqVRStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(
  VRPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES pqVRStarter.cxx
          ${MOC_SRCS}
          ${VR_SOURCE_FILES}
          ${RCS_SRCS} ${IFACE_SRCS})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVRConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVVRConfig.h
  ESCAPE_QUOTES IMMEDIATE)

# if VRPN is used then link to VRPN library
if(PARAVIEW_USE_VRPN)
  TARGET_LINK_LIBRARIES (VRPlugin "${VRPN_LIBRARY}")
endif(PARAVIEW_USE_VRPN)
