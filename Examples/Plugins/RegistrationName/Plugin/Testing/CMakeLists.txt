if (TARGET ParaView::paraview)
  set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)
  set(direct_data_tests
    TestRegistrationName
    TestRegistrationNameChangeFile)

  set(test_scripts)
  foreach(test_name ${direct_data_tests})
    set(${test_name}_USES_DIRECT_DATA ON)
    list(APPEND test_scripts ${test_name}.xml)
  endforeach()

  paraview_add_client_tests(
    LOAD_PLUGIN RegistrationName
    PLUGIN_PATH $<TARGET_FILE_DIR:RegistrationName>
    TEST_SCRIPTS ${test_scripts})

  foreach (test_name ${direct_data_tests})
    set_tests_properties(pv.${test_name}
      PROPERTIES
        ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")
  endforeach()
endif()
