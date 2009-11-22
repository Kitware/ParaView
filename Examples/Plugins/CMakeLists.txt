
SUBDIRS (
  Filter
  ParametricSource
  SMProxy
  Representation
  HiddenLinesRemoval
)

IF(PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(
    Autostart
    DisplayPanelDecorator
    DockWidget
    GUIObjectPanel
    GUIToolBar
    GUIView
    Reader
    SourceToolbar
    Writer
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)


