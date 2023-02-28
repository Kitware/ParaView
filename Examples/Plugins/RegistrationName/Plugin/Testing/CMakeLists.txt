if (TARGET ParaView::paraview)
  set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)
  set (TestRegistrationName_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN RegistrationName
    PLUGIN_PATH $<TARGET_FILE_DIR:RegistrationName>
    TEST_SCRIPTS TestRegistrationName.xml)
  set_tests_properties(
    pv.TestRegistrationName PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")
endif()
