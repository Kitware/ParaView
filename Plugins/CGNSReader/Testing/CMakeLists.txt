include(ParaViewTestingMacros)

set(_SUFFIX "")
if(VISIT_BUILD_READER_CGNS)
  set(_SUFFIX "${_SUFFIX}_WithVisitBridge")
endif()

set(CGNS_READER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/CGNSReader-5blocks_cgns${_SUFFIX}.xml)

set(CGNSReader-5blocks_cgns${_SUFFIX}_BREAK TRUE)

if (PARAVIEW_BUILD_QT_GUI)
  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CGNS_READER_TESTS}
    LOAD_PLUGIN "CGNSReader")
  set_property(TEST "pv.CGNSReader-5blocks_cgns${_SUFFIX}" PROPERTY PASS_REGULAR_EXPRESSION "client0 process exited with code 0")

  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CGNS_READER_TESTS}
    LOAD_PLUGIN "CGNSReader")
  set_property(TEST "pvcs.CGNSReader-5blocks_cgns${_SUFFIX}" PROPERTY PASS_REGULAR_EXPRESSION "server process exited with code 0")
endif()
