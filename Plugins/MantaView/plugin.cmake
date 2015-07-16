if (BUILD_SHARED_LIBS AND (VTK_RENDERING_BACKEND STREQUAL "OpenGL"))
  pv_plugin(MantaView
    DESCRIPTION "Manta Ray-Cast View")
endif()
