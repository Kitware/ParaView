ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII0.png}"
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII1.png}"
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII2.png}"
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII3.png}"
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII4.png}"
  "DATA{Data/Baseline/EnSightCombinedReaderElementsASCII5.png}")

set(tests
  "${CMAKE_CURRENT_SOURCE_DIR}/EnSightCombinedReaderElementsASCII.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/EnSightCombinedReaderElementsSOS.xml")

paraview_add_client_tests(
  LOAD_PLUGIN "EnSightGoldCombinedReader"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS ${tests})
