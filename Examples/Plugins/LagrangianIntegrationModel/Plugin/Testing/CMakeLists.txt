if (TARGET ParaView::paraview)
  set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)

  set (TestIntegrationModelExample_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN LagrangianIntegrationModel
    PLUGIN_PATH $<TARGET_FILE_DIR:LagrangianIntegrationModel>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestIntegrationModelExample.xml)
  set_tests_properties(pv.TestIntegrationModelExample PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")

  set (TestIntegrationModelExampleInteractionChange_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN LagrangianIntegrationModel
    PLUGIN_PATH $<TARGET_FILE_DIR:LagrangianIntegrationModel>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestIntegrationModelExampleInteractionChange.xml)
  set_tests_properties(pv.TestIntegrationModelExampleInteractionChange PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")

  if (PARAVIEW_USE_MPI)
    set (TestIntegrationModelExampleDistributed_USES_DIRECT_DATA ON)
    paraview_add_client_server_tests(
      LOAD_PLUGIN LagrangianIntegrationModel
      PLUGIN_PATH $<TARGET_FILE_DIR:LagrangianIntegrationModel>
      BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
      TEST_SCRIPTS TestIntegrationModelExampleDistributed.xml)
    set_tests_properties(pvcs.TestIntegrationModelExampleDistributed PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")
  endif()

endif()
