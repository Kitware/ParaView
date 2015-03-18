if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(EyeDomeLighting
    # provide a description for the plugin.
    DESCRIPTION "Add 3D View with eye-dome Lighting support"
    PLUGIN_NAMES EyeDomeLightingView
    DEFAULT_ENABLED)
endif()
