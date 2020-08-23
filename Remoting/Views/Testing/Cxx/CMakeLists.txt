vtk_add_test_cxx(vtkRemotingViewsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestComparativeAnimationCueProxy.cxx
  TestImageScaleFactors.cxx
  TestParaViewPipelineControllerWithRendering.cxx
  TestProxyManagerUtilities.cxx
  TestSystemCaps.cxx
  TestTransferFunctionManager.cxx
  TestTransferFunctionPresets.cxx)

vtk_add_test_cxx(vtkRemotingViewsCxxTests tests
  NO_VALID
  TestParaViewPipelineController.cxx)

vtk_test_cxx_executable(vtkRemotingViewsCxxTests tests)
