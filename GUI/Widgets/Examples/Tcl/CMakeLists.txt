SUBDIRS(
  HelloWorld
  WidgetsTour
  WindowWithPanels
)

IF(KWWidgets_BUILD_VTK_WIDGETS)
  SUBDIRS(
    MedicalImageViewer
    PolygonalObjectViewer
    )
ENDIF(KWWidgets_BUILD_VTK_WIDGETS)
  
