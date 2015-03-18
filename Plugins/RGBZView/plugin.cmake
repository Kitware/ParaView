if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(RGBZView
    # provide a description for the plugin.
    DESCRIPTION "3D View that dump into a file the RGB and Z buffer for each render."
    DEFAULT_ENABLED)
endif()
