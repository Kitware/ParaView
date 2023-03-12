if (NOT PARAVIEW_USE_QT OR NOT BUILD_SHARED_LIBS)
  return()
endif()

set(_paraview_add_tests_default_test_data_target ParaViewData)

ExternalData_Expand_Arguments(ParaViewData _
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/openCFS/Single_Region_Opt.cfs}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/openCFS/Mesh_Only.cfs}"
)

set(module_tests
   CFSReaderTest.xml
   MeshOnlyTest.xml)

paraview_add_client_tests(
  LOAD_PLUGIN   "CFSReader"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX CFSReaderPlugin::pv
  TEST_SCRIPTS  ${module_tests})

paraview_add_client_server_tests(
  LOAD_PLUGIN   "CFSReader"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX CFSReaderPlugin::pvcs
  TEST_SCRIPTS  ${module_tests})

paraview_add_client_server_render_tests(
  LOAD_PLUGIN   "CFSReader"
  BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  PREFIX CFSReaderPlugin::pvcsr
  TEST_SCRIPTS  ${module_tests})
