SUBDIRS(
  Callbacks
  HelloWorld
  MedicalImageViewer
  PolygonalObjectViewer
  WidgetsTour
  WindowWithPanels
)

OPTION(KWWidgets_USE_SOVIEWER "Use the SOViewer library and build the corresponding support classes and examples." OFF)
MARK_AS_ADVANCED(KWWidgets_USE_SOVIEWER)
IF(KWWidgets_USE_SOVIEWER)
  SUBDIRS(SOViewer)
ENDIF(KWWidgets_USE_SOVIEWER)
