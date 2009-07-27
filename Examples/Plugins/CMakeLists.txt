
SUBDIRS (
  Filter
  ParametricSource
  SMProxy
  Representation
  HiddenLinesRemoval
)

IF(PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(
    GUIToolBar
    Reader
    GUIObjectPanel
    GUIView
    Writer
    Autostart
    DisplayPanelDecorator
    SourceToolbar
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)


