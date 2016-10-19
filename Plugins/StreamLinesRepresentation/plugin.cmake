if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  pv_plugin(StreamLinesRepresentation
    DESCRIPTION "Add animated Stream Lines representation for any type of dataset"
    DEFAULT_ENABLED)
endif()
