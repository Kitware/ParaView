if (BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/BagPlots_A.png}"
    "DATA{Data/Baseline/BagPlots_B.png}"
    "DATA{Data/Baseline/BagPlots_B_1.png}"
    "DATA{Data/Baseline/BagPlotMatrixView_A.png}")

  set(BagPlots_METHOD LOOSE_VALID)

  paraview_add_client_tests(
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/CheckPluginVersionDescription.xml")
  paraview_add_client_tests(
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/BagPlots.xml")
  set (BagPlots_FORCE_SERIAL TRUE) # Chart interaction work better in serial
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
