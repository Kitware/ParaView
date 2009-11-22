# Example plugin demonstrating how to add a dock panel to ParaView.
# This plugin adds the panel ExampleDockPanel to the client.

QT4_WRAP_CPP(MOC_SRCS ExampleDockPanel.h)
QT4_WRAP_UI(UI_SRCS ExampleDockPanel.ui)

ADD_PARAVIEW_DOCK_WINDOW(
  OUTIFACES
  OUTSRCS
  CLASS_NAME ExampleDockPanel
  DOCK_AREA Right)

ADD_PARAVIEW_PLUGIN(ExampleDockPanel "1.0"
  GUI_INTERFACES ${OUTIFACES}
  GUI_SOURCES ${OUTSRCS} ${MOC_SRCS} ${UI_SRCS} ExampleDockPanel.cxx)

