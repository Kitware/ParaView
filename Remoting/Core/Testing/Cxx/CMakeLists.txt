vtk_add_test_cxx(vtkRemotingCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestPVArrayInformation.cxx
  TestPartialArraysInformation.cxx
  TestSpecialDirectories.cxx
  )

vtk_test_cxx_executable(vtkRemotingCoreCxxTests tests)
