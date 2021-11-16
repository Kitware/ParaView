vtk_add_test_cxx(vtkRemotingApplicationCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestExecutableRunner.cxx
  )

vtk_test_cxx_executable(vtkRemotingApplicationCxxTests tests)
