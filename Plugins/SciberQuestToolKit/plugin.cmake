if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  pv_plugin(SciberQuestToolKit
    DESCRIPTION
      "SciberQuestToolKit - Domain specific visualization tools for magnetospheric sciences."
    DEFAULT_ENABLED)
endif()
