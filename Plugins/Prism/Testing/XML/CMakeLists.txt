if (BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/proxium.asc}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/proxium.ses.mat.1.301}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/SESAMEConversions.xml}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/SymmetricImpact2.exo}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/SymmetricImpact/SymmetricImpact3.exo}"
    )

  set(paraview_prism_baseline_dir "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline")

  set(prism_test_scripts
    LANLLoadSESAME.xml
    LoadConversions.xml
    SandiaLoadSESAME.xml
    SESAMEContouring.xml
    SimulationData.xml
    SimulationDataCellDataToPointData.xml
    SimulationDataMulti.xml
    SimulationDataSelections.xml
    ViewAspectRatio.xml
    ViewLogScaling.xml
    ViewThresholding.xml
    )

  paraview_add_client_tests(
    LOAD_PLUGIN "Prism"
    BASELINE_DIR "${paraview_prism_baseline_dir}"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${prism_test_scripts}")

  if (PARAVIEW_USE_MPI)
    # Selections don't work in crs mode.
    set(SimulationDataSelections_DISABLE_CRS TRUE)

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
