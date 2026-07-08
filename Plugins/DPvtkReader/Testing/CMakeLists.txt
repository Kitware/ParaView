set(_paraview_add_tests_default_test_data_target DPvtkReaderData)
ExternalData_Expand_Arguments("${_paraview_add_tests_default_test_data_target}" _
  "DATA{Data/hohler-stilp-2ranks-5cycles.dpvtk}")

paraview_add_client_tests(
  LOAD_PLUGIN "DPvtkReader"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS TestDPvtkMultiBlockDataReader.xml)

if (PARAVIEW_USE_MPI)
  paraview_add_client_server_tests(
    LOAD_PLUGIN "DPvtkReader"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS TestDPvtkMultiBlockDataReader.xml)
endif ()

ExternalData_add_target("${_paraview_add_tests_default_test_data_target}")
