if (PARAVIEW_BUILD_QT_GUI)
  set (TestXML_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN ElevationFilter
    PLUGIN_PATH $<TARGET_FILE_DIR:ElevationFilter>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestXML.xml)
endif()
