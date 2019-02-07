if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/BagPlots.png}"
    "DATA{Data/Baseline/BagPlotsSelection.png}"
    "DATA{Data/Baseline/BagPlotMatrixView.png}"
    "DATA{Data/Baseline/BagPlotMatrixView_A.png}")

  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlots.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlotsSelection.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/FunctionalBagPlots.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlotMatrixView.xml)
endif()
