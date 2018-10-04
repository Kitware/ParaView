include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  paraview_test_load_data(NetCDFTimeAnnotationPlugin tos_O1_2001-2002.nc)

  add_client_tests("pv"
    LOAD_PLUGIN "NetCDFTimeAnnotationPlugin"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/NetCDFTimeAnnotationFilter.xml)

  add_client_server_tests("pvcs"
    LOAD_PLUGIN "NetCDFTimeAnnotationPlugin"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/NetCDFTimeAnnotationFilter.xml)
endif()
