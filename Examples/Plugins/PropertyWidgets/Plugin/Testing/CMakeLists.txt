if (TARGET ParaView::paraview)
  set(TestPropertyWidgetsDecorator_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN PropertyWidgets
    PLUGIN_PATH $<TARGET_FILE_DIR:PropertyWidgets>
    TEST_SCRIPTS TestPropertyWidgetsDecorator.xml)
endif()
