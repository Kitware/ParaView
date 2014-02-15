
# Example plugin demonstrating how to add a dock panel to ParaView.
# This plugin adds the panel ExampleDockPanel to the client.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(MOC_SRCS ExampleDockPanel.h)
    QT5_WRAP_UI(UI_SRCS ExampleDockPanel.ui)
  ELSE ()
    QT4_WRAP_CPP(MOC_SRCS ExampleDockPanel.h)
    QT4_WRAP_UI(UI_SRCS ExampleDockPanel.ui)
  ENDIF ()

  ADD_PARAVIEW_DOCK_WINDOW(
    OUTIFACES
    OUTSRCS
    CLASS_NAME ExampleDockPanel
    DOCK_AREA Right)

  ADD_PARAVIEW_PLUGIN(ExampleDockPanel "1.0"
                      GUI_INTERFACES ${OUTIFACES}
                      GUI_SOURCES ${OUTSRCS} ${MOC_SRCS} ${UI_SRCS} ExampleDockPanel.cxx)
ENDIF(PARAVIEW_BUILD_QT_GUI)
