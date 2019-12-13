if (PARAVIEW_USE_QT)
  set(_paraview_add_tests_default_test_data_target DigitalRockPhysicsData)
  ExternalData_Expand_Arguments("${_paraview_add_tests_default_test_data_target}" _
    "DATA{Data/rocksample.vti}")

  paraview_add_client_tests(
    LOAD_PLUGIN "DigitalRockPhysics"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS DigitalRockPhysicsAnalysisFilter.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "DigitalRockPhysics"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS DigitalRockPhysicsExplodeFilter.xml)

  if (PARAVIEW_USE_MPI)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "DigitalRockPhysics"
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS DigitalRockPhysicsAnalysisFilter.xml)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "DigitalRockPhysics"
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS DigitalRockPhysicsExplodeFilter.xml)
   endif ()

   ExternalData_add_target("${_paraview_add_tests_default_test_data_target}")
endif ()
