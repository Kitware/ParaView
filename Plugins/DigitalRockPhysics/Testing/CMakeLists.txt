include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  paraview_test_load_data(DigitalRockPhysics rocksample.vti)

  add_client_tests("pv"
    LOAD_PLUGIN "DigitalRockPhysics"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/DigitalRockPhysicsAnalysisFilter.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "DigitalRockPhysics"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/DigitalRockPhysicsExplodeFilter.xml)

  if (PARAVIEW_USE_MPI)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "DigitalRockPhysics"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/DigitalRockPhysicsAnalysisFilter.xml)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "DigitalRockPhysics"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/DigitalRockPhysicsExplodeFilter.xml)
   endif()
endif()
