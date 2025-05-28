ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/extra_params.json}"
  "DATA{Data/gaussian_kernel.onnx}"
  "DATA{Data/onnx_no_time.json}"
  "DATA{Data/onnx_time.json}"
  "DATA{Data/temporal_range_onnx.json}"
  "DATA{Data/test_output.json}"
)

paraview_add_client_tests(
  PREFIX "ONNX"
  LOAD_PLUGIN "ONNXPlugin"
  TEST_SCRIPTS
    TestLoadState.xml
    TestONNXPlugin.xml
    TestOutputParameters.xml
    TestTemporalEntries.xml
    TestWidgets.xml
)
