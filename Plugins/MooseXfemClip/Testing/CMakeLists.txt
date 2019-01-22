if (PARAVIEW_BUILD_QT_GUI)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/,REGEX:init_solution_propagation_out\\.e.*}")

  paraview_add_client_tests(
    LOAD_PLUGIN "MooseXfemClip"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS TestMooseXfemClip.xml
  )
endif ()
