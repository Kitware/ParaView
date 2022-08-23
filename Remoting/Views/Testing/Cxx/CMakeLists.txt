vtk_add_test_cxx(vtkRemotingViewsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestComparativeAnimationCueProxy.cxx
  TestImageScaleFactors.cxx
  TestParaViewPipelineControllerWithRendering.cxx
  TestProxyManagerUtilities.cxx
  TestScalarBarPlacement.cxx
  TestSystemCaps.cxx
  TestTransferFunctionManager.cxx)

vtk_add_test_cxx(vtkRemotingViewsCxxTests tests
  NO_VALID
  TestParaViewPipelineController.cxx
  TestTransferFunctionPresets.cxx)

vtk_module_test_data(
  Data/RdPu.ct)

vtk_test_cxx_executable(vtkRemotingViewsCxxTests tests)
