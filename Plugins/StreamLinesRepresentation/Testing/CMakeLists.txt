if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS StreamLinesRepresentationColor.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS StreamLinesRepresentationThick.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS StreamLinesRepresentationTransform.xml)
endif ()
