if (BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    LOAD_PLUGIN "NodeEditor"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS TestNodeEditorInterface.xml)

  paraview_add_client_server_tests(
    LOAD_PLUGIN "NodeEditor"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS TestNodeEditorInterface.xml)
endif ()
