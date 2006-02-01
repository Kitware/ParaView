SUBDIRS(
  Callbacks
  HelloWorld
  MedicalImageViewer
  PolygonalObjectViewer
  WidgetsTour
  WindowWithPanels
)

IF(KWWidgets_USE_SOVIEWER)
  SUBDIRS(SOViewer)
ENDIF(KWWidgets_USE_SOVIEWER)
