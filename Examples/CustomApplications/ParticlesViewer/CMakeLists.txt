cmake_minimum_required(VERSION 2.8.8)
project(ParticlesViewer)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "ParticlesViewer example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

include(ParaViewBranding)
include(ParaViewPlugins)

# This application demostrates following concepts:
# * Building user interface through a plugin
# * Loading startup plugins
# * Changing default ParaView behavior for creating representations

SET (SOURCE_FILES
  ParticlesViewerStarter.cxx
  ParticlesViewerDisplayPolicy.cxx
)

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS
    ParticlesViewerStarter.h
    ParticlesViewerDisplayPolicy.h
  )
  QT5_WRAP_UI(UI_BUILT_SOURCES
    ParticlesViewerMainWindow.ui
  )
else ()
  QT4_WRAP_CPP(MOC_SRCS
    ParticlesViewerStarter.h
    ParticlesViewerDisplayPolicy.h
  )
  QT4_WRAP_UI(UI_BUILT_SOURCES
    ParticlesViewerMainWindow.ui
  )
endif ()

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS
  CLASS_NAME ParticlesViewerStarter
  STARTUP startApplication
  SHUTDOWN stopApplication)

ADD_PARAVIEW_PLUGIN(ParticlesViewerPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  GUI_SOURCES ${IFACE_SRCS} ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES})
target_link_libraries(ParticlesViewerPlugin
                      LINK_PRIVATE pqApplicationComponents)

#------------------------------------------------------------------------------
# Build the client
build_paraview_client(particles_viewer
    TITLE "Particles Viewer (ParaView-based Example Application)"
    ORGANIZATION  "Particles Viewer Coorporation"
    VERSION_MAJOR 1
    VERSION_MINOR 0
    VERSION_PATCH 0
    REQUIRED_PLUGINS ParticlesViewerPlugin
                     PointSprite_Plugin
    GUI_CONFIGURATION_XMLS
      ${CMAKE_CURRENT_SOURCE_DIR}/SupportedReader.xml
)
