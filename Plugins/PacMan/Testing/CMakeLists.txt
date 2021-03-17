set(module_tests
  LoadPlugins.xml)

if ((TARGET ParaView::paraview OR paraview_will_be_built) AND BUILD_SHARED_LIBS)
  paraview_add_client_tests("pv"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${MODULE_TESTS}
  )
endif ()
