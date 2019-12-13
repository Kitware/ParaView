set(module_tests
  AnalyzeReaderWriterPlugin.xml
  NiftiReaderWriterPlugin.xml)

if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)
  set(_paraview_add_tests_default_test_data_target ParaViewData)
  ExternalData_Expand_Arguments(ParaViewData _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/ANALYZE/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/minimal.nii}")

  paraview_add_client_tests(
    LOAD_PLUGIN   "AnalyzeNIfTIReaderWriter"
    BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS  ${module_tests})
endif ()
