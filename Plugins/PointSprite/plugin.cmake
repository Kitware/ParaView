if (TARGET vtkIOVisItBridge AND NOT BUILD_SHARED_LIBS)
  # this plugin doesn't work for static builds.
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
