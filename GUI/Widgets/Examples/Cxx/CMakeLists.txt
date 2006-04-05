SUBDIRS(
  Callbacks
  HelloWorld
  MedicalImageViewer
  PolygonalObjectViewer
  Theme
  WidgetsTour
  WindowWithPanels
)

IF(KWWidgets_USE_INTERNATIONALIZATION)
  SUBDIRS(Internationalization)
ENDIF(KWWidgets_USE_INTERNATIONALIZATION)

IF(KWWidgets_USE_SOVIEWER)
  SUBDIRS(SOViewer)
ENDIF(KWWidgets_USE_SOVIEWER)
