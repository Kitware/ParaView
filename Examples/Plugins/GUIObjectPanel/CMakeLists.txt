
# create a plugin that implements an object panel for a source proxy

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  QT4_WRAP_CPP(MOC_SRCS pqConePanel.h)
  QT4_ADD_RESOURCES(RCS_SRCS pqConePanel.qrc)

  # we implement a pqConePanel.h for the ConeSource
  ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS CLASS_NAME pqConePanel
                            XML_NAME ConeSource XML_GROUP sources)

  # create a plugin for this panel
  ADD_PARAVIEW_PLUGIN(GUIConePanel "1.0"
                      GUI_INTERFACES ${IFACES}
                      SOURCES pqConePanel.cxx ${MOC_SRCS} ${RCS_SRCS} ${IFACE_SRCS})
ENDIF(PARAVIEW_BUILD_QT_GUI)
