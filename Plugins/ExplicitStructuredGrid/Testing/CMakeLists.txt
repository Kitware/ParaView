if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/explicitStructuredGrid.vtu}")

  paraview_add_client_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridSmooth.xml
  )
  paraview_add_client_server_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridSmooth.xml
  )
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridSmooth.xml
  )

  paraview_add_client_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridReal.xml
  )
  paraview_add_client_server_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridReal.xml
  )
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "ExplicitStructuredGrid"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExplicitStructuredGridReal.xml
  )

endif()
