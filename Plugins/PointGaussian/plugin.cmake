if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  pv_plugin(PointGaussian
    DESCRIPTION "Render Gaussian blurs for points"
    DEFAULT_ENABLED)
endif()
