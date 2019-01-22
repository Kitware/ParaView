if (PARAVIEW_BUILD_QT_GUI)
  set(test_xmls
    InSituExodusPluginTest.xml
  )

  paraview_add_client_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${test_xmls}
    LOAD_PLUGIN "InSituExodus"
  )

endif ()
