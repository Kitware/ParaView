if(PARAVIEW_BUILD_QT_GUI)

  include(ParaViewTestingMacros)

  set(test_xmls
    ${CMAKE_CURRENT_SOURCE_DIR}/InSituExodusPluginTest.xml
  )

  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${test_xmls}
    LOAD_PLUGIN "InSituExodus"
  )

endif()
