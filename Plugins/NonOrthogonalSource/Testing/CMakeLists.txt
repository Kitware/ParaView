if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  set (xmls
    NonOrthogonalSliceView.xml
    NonOrthographicOrthographicView.xml
  )

  set (NonOrthogonalSliceView_FORCE_SERIAL TRUE)

  paraview_add_client_tests(
    LOAD_PLUGIN "NonOrthogonalSource"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${xmls}
  )
  paraview_add_client_server_tests(
    LOAD_PLUGIN "NonOrthogonalSource"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${xmls}
  )
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "NonOrthogonalSource"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${xmls}
  )
endif ()
