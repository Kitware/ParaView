if (BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/proxium.asc}")

  set(paraview_prism_baseline_dir "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline")

  set(prism_test_scripts
    SandiaLoadSESAME.xml
    )

  paraview_add_client_tests(
    LOAD_PLUGIN "Prism"
    BASELINE_DIR "${paraview_prism_baseline_dir}"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${prism_test_scripts}")

  if (PARAVIEW_USE_MPI)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "Prism"
      BASELINE_DIR "${paraview_prism_baseline_dir}"
      TEST_DATA_TARGET ParaViewData
      TEST_SCRIPTS "${prism_test_scripts}")

    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "Prism"
      BASELINE_DIR "${paraview_prism_baseline_dir}"
      TEST_DATA_TARGET ParaViewData
      TEST_SCRIPTS "${prism_test_scripts}")
  endif ()
endif ()
