if (TARGET ParaView::paraview)
  set (TestAutostart_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN Autostart
    PLUGIN_PATH $<TARGET_FILE_DIR:Autostart>
    TEST_SCRIPTS TestAutostart.xml)
endif()
