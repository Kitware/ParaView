
set (TraceStatisticsFilter_DISABLE_CS TRUE)
set (TraceStatisticsFilter_DISABLE_CRS TRUE)

paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS SaveLoadRemotePythonState.xml
  )
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS SaveLoadRemotePythonScript.xml
  )
