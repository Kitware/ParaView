if (TARGET ParaView::paraview)
  set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)
  set (TestReaderName_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN ReaderName
    PLUGIN_PATH $<TARGET_FILE_DIR:ReaderName>
    TEST_SCRIPTS TestReaderName.xml)
  set_tests_properties(
    pv.TestReaderName PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")
endif()
