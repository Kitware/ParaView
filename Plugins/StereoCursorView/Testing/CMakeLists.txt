SET (TESTS_WITH_INLINE_COMPARES "StereoCursorViewTest.xml")

ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/Baseline/StereoCursorView_a.png}"
  "DATA{Data/Baseline/StereoCursorView_b.png}"
  "DATA{Data/Baseline/StereoCursorView_c.png}"
  "DATA{Data/Baseline/StereoCursorView_d.png}")

paraview_add_client_tests(
  LOAD_PLUGIN "StereoCursorView"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${TESTS_WITH_INLINE_COMPARES}")

paraview_add_client_server_tests(
  LOAD_PLUGIN "StereoCursorView"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${TESTS_WITH_INLINE_COMPARES}")
