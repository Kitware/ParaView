# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# ParaView
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

QT4_WRAP_CPP(MOC_SRCS
  pqVRUIStarter.h
  ParaViewVRUI.h
  vruiThread.h)

SET (QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

SET(VRUI_SOURCE_FILES
  ParaViewVRUI.cxx
  ParaViewVRUI.h
  vruiPipe.cxx
  vruiPipe.h
  vruiServerState.cxx
  vruiServerState.h
  vruiThread.cxx
  vruiThread.h
  vruiTrackerState.cxx
  vruiTrackerState.h
  ${MOC_SRCS})

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqVRUIStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(
  VRUIPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES pqVRUIStarter.cxx
          ${MOC_SRCS}
          ${VRUI_SOURCE_FILES}
          ${RCS_SRCS}
          ${IFACE_SRCS})
