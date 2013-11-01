if(PARAVIEW_DATA_ROOT AND PARAVIEW_BUILD_QT_GUI)

  include(ParaViewTestingMacros)

  set(test_xmls
    ${CMAKE_CURRENT_SOURCE_DIR}/InSituExodusPluginTest.xml
  )

  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${test_xmls}
    LOAD_PLUGIN "InSituExodus"
  )

endif()
