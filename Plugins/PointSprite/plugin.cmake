# this plugin doesn't work for static builds.
if (BUILD_SHARED_LIBS)
  pv_plugin(PointSprite
    DESCRIPTION "Point Sprites"
# disabled currently due to build issues.
#    DEFAULT_ENABLED
)
endif()
