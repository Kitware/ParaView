include(ParaViewTestingMacros)
paraview_test_load_data(""
  init_solution_propagation_out.e
  init_solution_propagation_out.e-s002
  init_solution_propagation_out.e-s003
  init_solution_propagation_out.e-s004
  init_solution_propagation_out.e-s005
  init_solution_propagation_out.e-s006)

if(PARAVIEW_BUILD_QT_GUI)
  add_client_tests("pv"
    LOAD_PLUGIN "MooseXfemClip"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TestMooseXfemClip.xml
  )
endif()
