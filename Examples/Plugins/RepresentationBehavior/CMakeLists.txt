# TODO: update this plugin to use the pipeline controller instead.
# Create an auto-start plugin that will add custom representation behavior.
# Typicialy when a representation will be created, the behavior will
# try to set it representation to Surface and pick on of the scalar data array

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS
    pqRepresentationBehaviorStarter.h
    pqSurfaceRepresentationBehavior.h)
ELSE ()
  QT4_WRAP_CPP(MOC_SRCS
    pqRepresentationBehaviorStarter.h
    pqSurfaceRepresentationBehavior.h)
ENDIF ()

ADD_PARAVIEW_AUTO_START(
  IFACES
     IFACE_SRCS
  CLASS_NAME
     pqRepresentationBehaviorStarter
  STARTUP
     onStartup
  SHUTDOWN
     onShutdown)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(AutoSurfaceRepresentation "1.0"
   GUI_INTERFACES
      ${IFACES}
   SOURCES
      pqRepresentationBehaviorStarter.cxx
      pqSurfaceRepresentationBehavior.cxx
      ${MOC_SRCS} ${RCS_SRCS} ${IFACE_SRCS}
)
