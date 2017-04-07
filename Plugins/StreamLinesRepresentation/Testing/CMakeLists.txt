include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  add_client_tests("pv"
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StreamLinesRepresentationColor.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StreamLinesRepresentationThick.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "StreamLinesRepresentation"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StreamLinesRepresentationTransform.xml)
endif()
