vtk_add_test_cxx(vtkPVVTKExtensionsCoreCxxTests tests
  NO_VALID NO_OUTPUT
  TestSubsetInclusionLattice.cxx
  TestFileSequenceParser.cxx)

vtk_test_cxx_executable(vtkPVVTKExtensionsCoreCxxTests tests)
