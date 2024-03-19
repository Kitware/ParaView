if (TARGET ParaView::paraview)
  set (TestOverrideXMLOnly_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN OverrideXMLOnly
    PLUGIN_PATH $<TARGET_FILE_DIR:OverrideXMLOnly>
    TEST_SCRIPTS TestOverrideXMLOnly.xml)
endif()
