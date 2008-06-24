
SUBDIRS(Filter ParametricSource SMProxy)

IF(PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(GUIToolBar Reader GUIObjectPanel GUIView Writer Autostart)
ENDIF(PARAVIEW_BUILD_QT_GUI)


