# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# ParaView
if(NOT ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
endif()

# VRPN
if(NOT EXISTS VRPN_INCLUDE_DIR)
  find_path(VRPN_INCLUDE_DIR vrpn_Tracker.h)
endif()

find_library(VRPN_LIBRARY vrpn)

QT4_WRAP_CPP(MOC_SRCS
  pqVRPNStarter.h
  ParaViewVRPN.h)

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
  ${VRPN_INCLUDE_DIR}
  )

INCLUDE_DIRECTORIES(
  ${VRPN_INCLUDE_DIR})

SET( VRPN_SOURCE_FILES
  ParaViewVRPN.cxx
  ParaViewVRPN.h
  ${MOC_SRCS})

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqVRPNStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter

ADD_PARAVIEW_PLUGIN(
  VRPNPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES pqVRPNStarter.cxx
          ${MOC_SRCS}
          ${VRPN_SOURCE_FILES}
          ${RCS_SRCS} ${IFACE_SRCS})

TARGET_LINK_LIBRARIES (VRPNPlugin "${VRPN_LIBRARY}")
