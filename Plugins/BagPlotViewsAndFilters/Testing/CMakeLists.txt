if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/BagPlotMatrixView_A.png}")

  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/BagPlots.xml")
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/BagPlotsSelection.xml")
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/FunctionalBagPlots.xml")
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/BagPlotMatrixView.xml")
endif()
