if (NOT PARAVIEW_USE_QT OR NOT BUILD_SHARED_LIBS)
  return()
endif()

set(_paraview_add_tests_default_test_data_target ParaViewData)

set(module_tests
  ContourLabelRepresentationTest.xml)

paraview_add_client_tests(
  LOAD_PLUGIN   "ContourLabelPlugin"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX ContourLabelPlugin::pv
  TEST_SCRIPTS  ${module_tests})

paraview_add_client_server_tests(
  LOAD_PLUGIN   "ContourLabelPlugin"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX ContourLabelPlugin::pvcs
  TEST_SCRIPTS  ${module_tests})

paraview_add_client_server_render_tests(
  LOAD_PLUGIN   "ContourLabelPlugin"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX ContourLabelPlugin::pvcsr
  TEST_SCRIPTS  ${module_tests})
