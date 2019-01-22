if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  set(_paraview_add_tests_default_test_data_target GmshReaderData)
  ExternalData_Expand_Arguments("${_paraview_add_tests_default_test_data_target}" _
    "DATA{Data/Gmsh,REGEX:viz_*}"
    "DATA{Data/naca0012,REGEX:.*}")

  paraview_add_client_tests(
    LOAD_PLUGIN "GmshReader"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GmshReaderTest.xml)

  paraview_add_client_server_tests(
    LOAD_PLUGIN "GmshReader"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GmshReaderMeshOnlyTest.xml)
endif()
