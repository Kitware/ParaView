vtk_add_test_cxx(vtkPVInSituCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestInSituPipelineException.cxx
  )

vtk_test_cxx_executable(vtkPVInSituCxxTests tests)
