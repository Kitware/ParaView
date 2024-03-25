set(xml_tests
  "TestEmulatedTimeSourceDummy.xml"
  "TestLiveSourceDummy.xml"
)

if (TARGET ParaView::paraview)
  set(TestEmulatedTimeSourceDummy_USES_DIRECT_DATA ON)
  set(TestLiveSourceDummy_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN LiveSourceDummy
    PLUGIN_PATH $<TARGET_FILE_DIR:LiveSourceDummy>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS ${xml_tests})
endif()
