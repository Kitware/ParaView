set(suffix "")
if (PARAVIEW_ENABLE_VISITBRIDGE AND VISIT_BUILD_READER_GMV)
  string(APPEND suffix "_WithVisitBridge")
endif ()

set(GMV_READER_TESTS
  GMVReaderBasicRead${suffix}.xml
  GMVReaderInvalidFiles${suffix}.xml
)

# Increase timeout for BasicRead as the data can take some time to read
set("GMVReaderBasicRead${suffix}_TIMEOUT" 300)

set(_paraview_add_tests_default_test_data_target ParaViewData)
ExternalData_Expand_Arguments(ParaViewData _
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/,REGEX:.*}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/invalid/,REGEX:.*}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/partial-pyramid/,REGEX:.*}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/quadratic_cells/,REGEX:.*}")

paraview_add_client_tests(
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline"
  TEST_SCRIPTS ${GMV_READER_TESTS}
  TEST_DATA_TARGET ParaViewData
  LOAD_PLUGIN "GMVReader"
)
if (TEST "pv.GMVReaderInvalidFiles${suffix}")
  set_property(TEST "pv.GMVReaderInvalidFiles${suffix}"
    PROPERTY
      PASS_REGULAR_EXPRESSION "client0 process exited with code 0")
endif ()

  paraview_add_client_server_tests(
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline"
  TEST_SCRIPTS ${GMV_READER_TESTS}
  TEST_DATA_TARGET ParaViewData
  LOAD_PLUGIN "GMVReader"
)
if (TEST "pvcs.GMVReaderInvalidFiles${suffix}")
  set_property(TEST "pvcs.GMVReaderInvalidFiles${suffix}"
    PROPERTY
      PASS_REGULAR_EXPRESSION "server process exited with code 0")
endif ()
