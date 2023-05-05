if (TARGET ParaView::paraview)
  set(TestLiveSourceDummy_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN LiveSourceDummy
    PLUGIN_PATH $<TARGET_FILE_DIR:LiveSourceDummy>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestLiveSourceDummy.xml)
endif()
