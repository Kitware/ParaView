if (BUILD_SHARED_LIBS)

  #set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)
  paraview_add_client_tests(
      LOAD_PLUGIN "LegacyGhostCellsGenerator"
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_DATA_TARGET ParaViewData
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/LegacyGhostCellsGenerator.xml")

  paraview_add_client_server_tests(
    LOAD_PLUGIN "LegacyGhostCellsGenerator"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/LegacyGhostCellsGenerator.xml")

endif()
