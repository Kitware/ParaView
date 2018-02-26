include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  add_client_tests("pv"
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExtrusionRepresentationPointData.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExtrusionRepresentationCellData.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/BumpMapRepresentationPointData.xml)
endif()
