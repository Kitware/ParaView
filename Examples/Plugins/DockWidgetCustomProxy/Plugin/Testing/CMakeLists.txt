if (TARGET ParaView::paraview)
  set (TestWrite_USES_DIRECT_DATA ON)
  set (TestWrite_FORCE_SERIAL TRUE) # since this uses popup-menu
  paraview_add_client_tests(
    LOAD_PLUGIN DockWidgetCustomProxy
    PLUGIN_PATH $<TARGET_FILE_DIR:DockWidgetCustomProxy>
    TEST_SCRIPTS TestWrite.xml)
endif()
