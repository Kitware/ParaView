if(PARAVIEW_BUILD_QT_GUI)
  pv_plugin(MobileRemoteControl
    DESCRIPTION "Use a mobile device to view the ParaView scene and control the camera."
    DEFAULT_ENABLED)
endif()
