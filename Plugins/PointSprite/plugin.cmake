if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(PointSprite
    DESCRIPTION "Point Sprites"
    DEFAULT_ENABLED
    PLUGIN_NAMES "PointSprite_Plugin"
    )
endif()
