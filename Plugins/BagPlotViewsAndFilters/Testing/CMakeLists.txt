include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  add_client_tests("pv"
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlots.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlotsSelection.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/FunctionalBagPlots.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "BagPlotViewsAndFilters"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BagPlotMatrixView.xml)
endif()
