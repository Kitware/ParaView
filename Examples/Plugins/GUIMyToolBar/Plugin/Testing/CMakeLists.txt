set(xml_tests
  "TestAppendToolBarActions.xml"
)

if (TARGET ParaView::paraview)
  set(TestAppendToolBarActions_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN GUIMyToolBar
    PLUGIN_PATH $<TARGET_FILE_DIR:GUIMyToolBar>
    TEST_SCRIPTS ${xml_tests})
endif()
