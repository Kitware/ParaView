if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(UncertaintyRendering
    DESCRIPTION "Add uncertainty visualization support"
    DEFAULT_ENABLED)
endif()
