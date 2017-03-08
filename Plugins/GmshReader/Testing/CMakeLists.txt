include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  paraview_test_load_data_dirs(GmshReader
    Gmsh
    Gmsh/naca0012)

  add_client_tests("pv"
    LOAD_PLUGIN "GmshReader"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GmshReaderTest.xml)

  add_client_server_tests("pvcs"
    LOAD_PLUGIN "GmshReader"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GmshReaderMeshOnlyTest.xml)
endif()
