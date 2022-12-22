if (BUILD_SHARED_LIBS)

  # This are smoke test that will not test actual XR
  # rendering but just ensure that ParaView do not crash when
  # loading and using basic fonctionnalities of the plugin.
  # Errors are expected.
  paraview_add_client_tests(
    SMTESTING_ALLOW_ERRORS
    LOAD_PLUGIN "XRInterface"
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/XRInterfaceSendToXR.xml")

  if (TARGET VTK::RenderingOpenXR AND TARGET VTK::RenderingOpenVR)
    paraview_add_client_tests(
      SMTESTING_ALLOW_ERRORS
      LOAD_PLUGIN "XRInterface"
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/XRInterfaceSendAndSwitch.xml")
  endif()
endif()
