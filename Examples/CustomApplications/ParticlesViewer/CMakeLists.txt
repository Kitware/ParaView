
cmake_minimum_required(VERSION 2.6)

project(ParticlesViewer)

FIND_PACKAGE(ParaView REQUIRED)
FIND_PACKAGE(Qt4)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

# This application demostrates following concepts:
# * Building user interface through a plugin
# * Loading startup plugins
# * Changing default ParaView behavior for creating representations

SET (SOURCE_FILES
  ParticlesViewerStarter.cxx
  ParticlesViewerDisplayPolicy.cxx
)

QT4_WRAP_CPP(MOC_SRCS
  ParticlesViewerStarter.h
  ParticlesViewerDisplayPolicy.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ParticlesViewerMainWindow.ui
)

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS
  CLASS_NAME ParticlesViewerStarter
  STARTUP startApplication
  SHUTDOWN stopApplication)

ADD_PARAVIEW_PLUGIN(ParticlesViewerPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  GUI_SOURCES ${IFACE_SRCS} ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES})

TARGET_LINK_LIBRARIES(ParticlesViewerPlugin pqApplicationComponents)

#------------------------------------------------------------------------------
# Build the client
build_paraview_client(particles_viewer
    TITLE "Particles Viewer (ParaView-based Example Application)"
    ORGANIZATION  "Particles Viewer Coorporation"
    VERSION_MAJOR 1
    VERSION_MINOR 0
    VERSION_PATCH 0
    REQUIRED_PLUGINS ParticlesViewerPlugin
                     CSCS_PointSprite_Plugin
    GUI_CONFIGURATION_XMLS
      ${CMAKE_CURRENT_SOURCE_DIR}/SupportedReader.xml
)

TARGET_LINK_LIBRARIES(particles_viewer ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} vtkCommon vtkPVCommon pqCore pqComponents)
