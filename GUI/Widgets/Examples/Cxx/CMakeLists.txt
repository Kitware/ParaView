SUBDIRS(
  Callbacks
  HelloWorld
  Theme
  WidgetsTour
  WindowWithPanels
  WizardDialog
)

IF(KWWidgets_USE_INTERNATIONALIZATION)
  SUBDIRS(Internationalization)
ENDIF(KWWidgets_USE_INTERNATIONALIZATION)

IF(KWWidgets_BUILD_VTK_WIDGETS)
  SUBDIRS(
    MedicalImageViewer
    PolygonalObjectViewer
    )
  IF(KWWidgets_USE_SOVIEWER)
    SUBDIRS(SOViewer)
  ENDIF(KWWidgets_USE_SOVIEWER)
ENDIF(KWWidgets_BUILD_VTK_WIDGETS)
