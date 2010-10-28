PROJECT(ParaViewExamples)

# VTK examples now require CMake 2.6 or higher.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

SUBDIRS (
  Filter
  ParametricSource
  SMProxy
  Representation
  HiddenLinesRemoval
)

IF(PARAVIEW_USE_VISITBRIDGE)
  SUBDIRS(
  VisItReader
  )
ENDIF(PARAVIEW_USE_VISITBRIDGE)

IF(PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(
    Autostart
    DisplayPanelDecorator
    DockWidget
    GUIObjectPanel
    GUIToolBar
    GUIView
    Reader
    RenderPassViews
    SourceToolbar
    Writer
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)


