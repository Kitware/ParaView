vtk_add_test_cxx(vtkPVServerManagerRenderingCxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestImageScaleFactors.cxx
  TestParaViewPipelineControllerWithRendering.cxx
  TestTransferFunctionManager.cxx
  TestTransferFunctionPresets.cxx
  )
vtk_test_cxx_executable(vtkPVServerManagerRenderingCxxTests tests)
