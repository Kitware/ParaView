if (BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/blow.vtk}")

  paraview_add_client_tests(
    LOAD_PLUGIN "TemporalParallelismScriptGenerator"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS TemporalParallelism.xml)
endif ()
