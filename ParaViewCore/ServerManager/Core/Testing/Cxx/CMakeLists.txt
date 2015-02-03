paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestSessionProxyManager.cxx
  TestSettings.cxx
  )
paraview_add_test_cxx(${vtk-module}CxxTests tmp_tests
  NO_DATA NO_VALID
  TestParaViewPipelineController.cxx
  )
list(APPEND tests
  ${tmp_tests})
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
