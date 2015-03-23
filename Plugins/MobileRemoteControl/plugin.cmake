if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  if(PARAVIEW_BUILD_QT_GUI)

    set(_mobile_remote_DEFAULT_ENABLED)
    if (BUILD_SHARED_LIBS)
      set(_mobile_remote_DEFAULT_ENABLED DEFAULT_ENABLED)
    endif()

    pv_plugin(MobileRemoteControl
      DESCRIPTION "Use a mobile device to view the ParaView scene and control the camera."
      ${_mobile_remote_DEFAULT_ENABLED}
    )
  endif()
endif()
