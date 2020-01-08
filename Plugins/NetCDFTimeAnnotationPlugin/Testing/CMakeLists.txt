if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/tos_O1_2001-2002.nc}")

  paraview_add_client_tests(
    LOAD_PLUGIN "NetCDFTimeAnnotationPlugin"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS NetCDFTimeAnnotationFilter.xml)

  paraview_add_client_server_tests(
    LOAD_PLUGIN "NetCDFTimeAnnotationPlugin"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS NetCDFTimeAnnotationFilter.xml)
endif ()
