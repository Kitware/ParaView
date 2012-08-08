# this plugin doesn't work for static builds.
if (BUILD_SHARED_LIBS)
  pv_plugin(PointSprite
    DESCRIPTION "Point Sprites"
    DEFAULT_ENABLED
    PLUGIN_NAMES "PointSprite_Plugin"
    )
else()
  message(STATUS
    "PointSprite plugin is not available in static builds.")
endif()
