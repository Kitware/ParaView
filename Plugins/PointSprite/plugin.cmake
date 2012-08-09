# this plugin doesn't work for static builds.
if (BUILD_SHARED_LIBS)
  if (TARGET vtkIOVisItBridge)
    message(STATUS
      "VisitBridge is enabled. "
      "Due to current limitations, PointSprite plugin cannot be built with "
      "VisitBridge is enabled, hence disabling it.")
  else()
    pv_plugin(PointSprite
      DESCRIPTION "Point Sprites"
      DEFAULT_ENABLED
      PLUGIN_NAMES "PointSprite_Plugin"
      )
  endif()
else()
  message(STATUS
    "PointSprite plugin is not available in static builds.")
endif()
