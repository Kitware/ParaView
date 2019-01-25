vtk_add_test_cxx(vtkPVServerManagerCoreCxxTests tests
  NO_DATA NO_VALID
  TestAdjustRange.cxx
  TestSelfGeneratingSourceProxy.cxx
  TestSessionProxyManager.cxx
  TestSettings.cxx
  TestRecreateVTKObjects.cxx
  )

vtk_add_test_cxx(vtkPVServerManagerCoreCxxTests tmp_tests
  NO_VALID
  TestParaViewPipelineController.cxx
  )
list(APPEND tests
  ${tmp_tests})

vtk_test_cxx_executable(vtkPVServerManagerCoreCxxTests tests)
