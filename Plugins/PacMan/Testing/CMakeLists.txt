set(module_tests
  LoadPacManPlugin.xml)

if (BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${module_tests}
  )
endif ()
