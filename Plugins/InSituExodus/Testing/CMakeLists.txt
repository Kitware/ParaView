if (PARAVIEW_USE_QT)
  set(test_xmls
    InSituExodusPluginTest.xml
  )

  paraview_add_client_tests(
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${test_xmls}
    LOAD_PLUGIN "InSituExodus"
  )

endif ()
