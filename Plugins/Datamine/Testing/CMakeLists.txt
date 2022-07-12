ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/datamine_smallpt.dm}"
  "DATA{Data/datamine_smalltr.dm}"
  )

set (DatamineReader_FORCE_SERIAL TRUE)  # since this uses popup-menu

paraview_add_client_tests(
  LOAD_PLUGIN "Datamine"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS DatamineReader.xml)
