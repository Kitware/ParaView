if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(SurfaceLIC
    DESCRIPTION "Add Surface-LIC vector visualization support"
    DEFAULT_ENABLED)
endif()
