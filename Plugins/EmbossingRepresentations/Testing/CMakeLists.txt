include(ParaViewTesting)

if (BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExtrusionRepresentationPointData.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExtrusionRepresentationCellData.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BumpMapRepresentationPointData.xml)
endif()
